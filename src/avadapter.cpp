/*
 * Copyright (C) 2020-2022 Savoir-faire Linux Inc.
 * Author : Edric Ladent Milaret<edric.ladent - milaret @savoirfairelinux.com>
 * Author : Andreas Traczyk<andreas.traczyk @savoirfairelinux.com>
 * Author: Mingrui Zhang <mingrui.zhang@savoirfairelinux.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "avadapter.h"
#include "qtutils.h"

#include "api/newcodecmodel.h"
#include "api/newdevicemodel.h"

#ifdef Q_OS_LINUX
#include "xrectsel.h"
#endif

#include <QtConcurrent/QtConcurrent>
#include <QApplication>
#include <QPainter>
#include <QScreen>

AvAdapter::AvAdapter(LRCInstance* instance, QObject* parent)
    : QmlAdapterBase(instance, parent)
{
    connect(&lrcInstance_->avModel(),
            &lrc::api::AVModel::audioDeviceEvent,
            this,
            &AvAdapter::onAudioDeviceEvent);
    connect(&lrcInstance_->avModel(),
            &lrc::api::AVModel::rendererStarted,
            this,
            &AvAdapter::onRendererStarted);
}

// The top left corner of primary screen is (0, 0).
// For Qt, QScreen geometry contains x, y location relative to primary screen.
// The purpose of the function is to use calculate a boundingRect for virtual desktop
// to help screen sharing.
const QRect
AvAdapter::getAllScreensBoundingRect()
{
    auto screens = QGuiApplication::screens();

    // p0 is for x axis, p1 is for y axis,
    // points contain values that are the maximum positive and negative domain value
    QPoint p0(0, 0), p1(0, 0);

    for (auto scr : screens) {
        auto devicePixelRatio = scr->devicePixelRatio();
        auto screenRect = scr->geometry();

        if (screenRect.y() < 0 && p1.y() < abs(screenRect.y()))
            p1.setY(abs(screenRect.y()));
        else if (screenRect.y() >= 0
                 && p1.x() < screenRect.y() + screenRect.height() * devicePixelRatio)
            p1.setX(screenRect.y() + screenRect.height() * devicePixelRatio);

        if (screenRect.x() < 0 && p0.y() < abs(screenRect.x()))
            p0.setY(abs(screenRect.x()));
        else if (screenRect.x() >= 0
                 && p0.x() < screenRect.x() + screenRect.width() * devicePixelRatio)
            p0.setX(screenRect.x() + screenRect.width() * devicePixelRatio);
    }

    return QRect(-p0.y(), -p1.y(), p0.y() + p0.x(), p1.y() + p1.x());
}

void
AvAdapter::shareEntireScreen(int screenNumber)
{
    QScreen* screen = QGuiApplication::screens().at(screenNumber);
    if (!screen)
        return;
    QRect rect = screen->geometry();

    auto resource = lrcInstance_->getCurrentCallModel()
                        ->getDisplay(getScreenNumber(),
                                     rect.x(),
                                     rect.y(),
                                     rect.width() * screen->devicePixelRatio(),
                                     rect.height() * screen->devicePixelRatio());
    auto callId = lrcInstance_->getCurrentCallId();
    lrcInstance_->getCurrentCallModel()
        ->requestMediaChange(callId,
                             "video_0",
                             resource,
                             lrc::api::NewCallModel::MediaRequestType::SCREENSHARING,
                             false);
}

void
AvAdapter::shareAllScreens()
{
    const auto arrangementRect = getAllScreensBoundingRect();

    auto resource = lrcInstance_->getCurrentCallModel()->getDisplay(getScreenNumber(),
                                                                    arrangementRect.x(),
                                                                    arrangementRect.y(),
                                                                    arrangementRect.width(),
                                                                    arrangementRect.height());
    auto callId = lrcInstance_->getCurrentCallId();
    lrcInstance_->getCurrentCallModel()
        ->requestMediaChange(callId,
                             "video_0",
                             resource,
                             lrc::api::NewCallModel::MediaRequestType::SCREENSHARING,
                             false);
}

void
AvAdapter::captureScreen(int screenNumber)
{
    std::ignore = QtConcurrent::run([this, screenNumber]() {
        QScreen* screen = QGuiApplication::screens().at(screenNumber);
        if (!screen)
            return;
        /*
         * The screen window id is always 0.
         */
        auto pixmap = screen->grabWindow(0);

        QBuffer buffer;
        buffer.open(QIODevice::WriteOnly);
        pixmap.save(&buffer, "PNG");

        Q_EMIT screenCaptured(screenNumber, Utils::byteArrayToBase64String(buffer.data()));
    });
}

void
AvAdapter::captureAllScreens()
{
    std::ignore = QtConcurrent::run([this]() {
        auto screens = QGuiApplication::screens();

        QList<QPixmap> scrs;
        int width = 0, height = 0, currentPoint = 0;

        for (auto scr : screens) {
            QPixmap pix = scr->grabWindow(0);
            auto devicePixelRatio = scr->devicePixelRatio();
            width += scr->geometry().width() * devicePixelRatio;
            if (height < scr->geometry().height() * devicePixelRatio)
                height = scr->geometry().height() * devicePixelRatio;
            scrs << pix;
        }

        QPixmap final(width, height);
        QPainter painter(&final);
        final.fill(Qt::black);

        for (auto scr : scrs) {
            painter.drawPixmap(currentPoint, 0, scr.width(), scr.height(), scr);
            currentPoint += scr.width();
        }

        QBuffer buffer;
        buffer.open(QIODevice::WriteOnly);
        final.save(&buffer, "PNG");
        Q_EMIT screenCaptured(-1, Utils::byteArrayToBase64String(buffer.data()));
    });
}

void
AvAdapter::shareFile(const QString& filePath)
{
    auto callId = lrcInstance_->getCurrentCallId();
    if (!callId.isEmpty()) {
        lrcInstance_->getCurrentCallModel()
            ->requestMediaChange(callId,
                                 "video_0",
                                 filePath,
                                 lrc::api::NewCallModel::MediaRequestType::FILESHARING,
                                 false);
    }
}

void
AvAdapter::shareScreenArea(unsigned x, unsigned y, unsigned width, unsigned height)
{
#ifdef Q_OS_LINUX
    // xrectsel will freeze all displays too fast so that the call
    // context menu will not be closed even closed signal is emitted
    // use timer to wait until popup is closed
    QTimer::singleShot(100, [=]() mutable {
        x = y = width = height = 0;
        xrectsel(&x, &y, &width, &height);
        auto resource = lrcInstance_->getCurrentCallModel()->getDisplay(getScreenNumber(),
                                                                        x,
                                                                        y,
                                                                        width < 128 ? 128 : width,
                                                                        height < 128 ? 128 : height);
        auto callId = lrcInstance_->getCurrentCallId();
        lrcInstance_->getCurrentCallModel()
            ->requestMediaChange(callId,
                                 "video_0",
                                 resource,
                                 lrc::api::NewCallModel::MediaRequestType::SCREENSHARING,
                                 false);
    });
#else
    auto resource = lrcInstance_->getCurrentCallModel()->getDisplay(getScreenNumber(),
                                                                    x,
                                                                    y,
                                                                    width < 128 ? 128 : width,
                                                                    height < 128 ? 128 : height);
    auto callId = lrcInstance_->getCurrentCallId();
    lrcInstance_->getCurrentCallModel()
        ->requestMediaChange(callId,
                             "video_0",
                             resource,
                             lrc::api::NewCallModel::MediaRequestType::SCREENSHARING,
                             false);
#endif
}

void
AvAdapter::shareWindow(const QString& windowId)
{
    auto resource = lrcInstance_->getCurrentCallModel()->getDisplay(windowId);
    auto callId = lrcInstance_->getCurrentCallId();
    lrcInstance_->getCurrentCallModel()
        ->requestMediaChange(callId,
                             "video_0",
                             resource,
                             lrc::api::NewCallModel::MediaRequestType::SCREENSHARING,
                             false);
}

QString
AvAdapter::getSharingResource(int screenId = -2, const QString& windowId = "")
{
    if (screenId == -1) {
        const auto arrangementRect = getAllScreensBoundingRect();

        return lrcInstance_->getCurrentCallModel()->getDisplay(getScreenNumber(),
                                                               arrangementRect.x(),
                                                               arrangementRect.y(),
                                                               arrangementRect.width(),
                                                               arrangementRect.height());
    } else if (screenId > -1) {
        QScreen* screen = QGuiApplication::screens().at(screenId);
        if (!screen)
            return "";
        QRect rect = screen->geometry();

        return lrcInstance_->getCurrentCallModel()->getDisplay(getScreenNumber(),
                                                               rect.x(),
                                                               rect.y(),
                                                               rect.width()
                                                                   * screen->devicePixelRatio(),
                                                               rect.height()
                                                                   * screen->devicePixelRatio());
    } else if (!windowId.isEmpty()) {
        return lrcInstance_->getCurrentCallModel()->getDisplay(windowId);
    }

    return "";
}

void
AvAdapter::getListWindows()
{
    auto map = lrcInstance_->avModel().getListWindows();
    set_windowsNames(map.keys());
    set_windowsIds(map.values());
}

void
AvAdapter::stopSharing()
{
    auto callId = lrcInstance_->getCurrentCallId();
    if (!callId.isEmpty()) {
        lrcInstance_->getCurrentCallModel()
            ->requestMediaChange(callId,
                                 "video_0",
                                 lrcInstance_->avModel().getCurrentVideoCaptureDevice(),
                                 lrc::api::NewCallModel::MediaRequestType::CAMERA,
                                 muteCamera_);
    }
}

void
AvAdapter::startAudioMeter()
{
    lrcInstance_->startAudioMeter();
}

void
AvAdapter::stopAudioMeter()
{
    lrcInstance_->stopAudioMeter();
}

void
AvAdapter::onAudioDeviceEvent()
{
    auto& avModel = lrcInstance_->avModel();
    auto inputs = avModel.getAudioInputDevices().size();
    auto outputs = avModel.getAudioOutputDevices().size();
    Q_EMIT audioDeviceListChanged(inputs, outputs);
}

void
AvAdapter::onRendererStarted(const QString& id)
{
    auto callId = lrcInstance_->getCurrentCallId();
    auto callModel = lrcInstance_->getCurrentCallModel();
    auto renderDevice = callModel->getCurrentRenderedDevice(callId);
    set_currentRenderingDeviceId(id);
    set_currentRenderingDeviceType(renderDevice.type);
}

int
AvAdapter::getScreenNumber() const
{
    int display = 0;

#ifdef Q_OS_LINUX
    // Get display
    QString display_env {getenv("DISPLAY")};
    if (!display_env.isEmpty()) {
        auto list = display_env.split(':', Qt::SkipEmptyParts);
        // Should only be one display, so get the first one
        if (list.size() > 0) {
            display = list.at(0).toInt();
        }
    }
#endif
    return display;
}

void
AvAdapter::setDeviceName(const QString& deviceName)
{
    lrcInstance_->getCurrentAccountInfo().deviceModel->setCurrentDeviceName(deviceName);
}

void
AvAdapter::enableCodec(unsigned int id, bool isToEnable)
{
    lrcInstance_->getCurrentAccountInfo().codecModel->enable(id, isToEnable);
}

void
AvAdapter::increaseCodecPriority(unsigned int id, bool isVideo)
{
    lrcInstance_->getCurrentAccountInfo().codecModel->increasePriority(id, isVideo);
}

void
AvAdapter::decreaseCodecPriority(unsigned int id, bool isVideo)
{
    lrcInstance_->getCurrentAccountInfo().codecModel->decreasePriority(id, isVideo);
}

bool
AvAdapter::getHardwareAcceleration()
{
    return lrcInstance_->avModel().getHardwareAcceleration();
}
void
AvAdapter::setHardwareAcceleration(bool accelerate)
{
    lrcInstance_->avModel().setHardwareAcceleration(accelerate);
}
