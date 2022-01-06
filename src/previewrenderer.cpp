/*
 * Copyright (C) 2020-2022 Savoir-faire Linux Inc.
 * Author: Andreas Traczyk <andreas.traczyk@savoirfairelinux.com>
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

#include "previewrenderer.h"

PreviewRenderer::PreviewRenderer(QQuickItem* parent)
    : QQuickPaintedItem(parent)
{
    setAntialiasing(true);
    setFillColor(Qt::black);
    setRenderTarget(QQuickPaintedItem::FramebufferObject);
    setPerformanceHint(QQuickPaintedItem::FastFBOResizing);

    connect(this, &PreviewRenderer::lrcInstanceChanged, [this] {
        if (lrcInstance_)
            previewFrameUpdatedConnection_ = connect(lrcInstance_->renderer(),
                                                     &RenderManager::distantFrameUpdated,
                                                     [this](const QString& id) {
                                                         if (rendererId_ == id && isVisible())
                                                             update(QRect(0, 0, width(), height()));
                                                     });
    });
}

PreviewRenderer::~PreviewRenderer()
{
    disconnect(previewFrameUpdatedConnection_);
}

void
PreviewRenderer::paint(QPainter* painter)
{
    lrcInstance_->renderer()->drawFrame(rendererId_, [this, painter](QImage* previewImage) {
        if (previewImage) {
            painter->setRenderHint(QPainter::Antialiasing);
            painter->setRenderHint(QPainter::SmoothPixmapTransform);

            auto aspectRatio = static_cast<qreal>(previewImage->width())
                               / static_cast<qreal>(previewImage->height());
            auto previewHeight = aspectRatio < 1 ? height() : width() / aspectRatio;
            auto previewWidth = aspectRatio < 1 ? previewHeight * aspectRatio : width();

            /* Instead of setting fixed size, we could get an x offset for the preview
             * but this would render the horizontal spacers in the parent widget useless.
             * e.g.
             * auto parent = qobject_cast<QWidget*>(this->parent());
             * auto xPos = (parent->width() - previewWidth) / 2;
             * setGeometry(QRect(QPoint(xPos, this->pos().y()),
             *             QSize(previewWidth, previewHeight)));
             */
            setWidth(previewWidth);
            setHeight(previewHeight);

            // If the given size is empty, this function returns a null image.
            QImage scaledPreview;
#if defined(Q_OS_MACOS)

            scaledPreview = previewImage
                                ->scaled(size().toSize(),
                                         Qt::KeepAspectRatio,
                                         Qt::SmoothTransformation)
                                .rgbSwapped();
#else
                scaledPreview = previewImage->scaled(size().toSize(),
                                                     Qt::KeepAspectRatio,
                                                     Qt::SmoothTransformation);
#endif
            painter->drawImage(QRect(0, 0, scaledPreview.width(), scaledPreview.height()),
                               scaledPreview);
        } else {
            paintBackground(painter);
        }
    });
}

void
PreviewRenderer::paintBackground(QPainter* painter)
{
    QBrush brush(Qt::black);
    QPainterPath path;
    path.addRect(QRect(0, 0, width(), height()));
    painter->fillPath(path, brush);
}

VideoCallPreviewRenderer::VideoCallPreviewRenderer(QQuickItem* parent)
    : PreviewRenderer(parent)
{
    setProperty("previewImageScalingFactor", 1.0);
}

VideoCallPreviewRenderer::~VideoCallPreviewRenderer() {}

void
VideoCallPreviewRenderer::paint(QPainter* painter)
{
    lrcInstance_->renderer()->drawFrame(get_rendererId(), [this, painter](QImage* previewImage) {
        if (previewImage) {
            auto scalingFactor = static_cast<qreal>(previewImage->height())
                                 / static_cast<qreal>(previewImage->width());
            setProperty("previewImageScalingFactor", scalingFactor);
            QImage scaledPreview;
#if defined(Q_OS_MACOS)
            scaledPreview = previewImage->scaled(size().toSize(), Qt::KeepAspectRatio).rgbSwapped();
#else
                scaledPreview = previewImage->scaled(size().toSize(), Qt::KeepAspectRatio);
#endif
            painter->drawImage(QRect(0, 0, scaledPreview.width(), scaledPreview.height()),
                               scaledPreview);
        }
    });
}

PhotoboothPreviewRender::PhotoboothPreviewRender(QQuickItem* parent)
    : PreviewRenderer(parent)
{
    connect(this, &PreviewRenderer::lrcInstanceChanged, [this] {
        if (lrcInstance_)
            connect(lrcInstance_->renderer(),
                    &RenderManager::distantRenderingStopped,
                    this,
                    &PhotoboothPreviewRender::renderingStopped,
                    Qt::UniqueConnection);
    });
}

QString
PhotoboothPreviewRender::takePhoto(int size)
{
    if (auto previewImage = lrcInstance_->renderer()->getPreviewFrame(get_rendererId())) {
#if defined(Q_OS_MACOS)
        return Utils::byteArrayToBase64String(
            Utils::QImageToByteArray(previewImage->copy().rgbSwapped()));
#else
        return Utils::byteArrayToBase64String(Utils::QImageToByteArray(previewImage->copy()));
#endif
    }
    return {};
}

void
PhotoboothPreviewRender::paint(QPainter* painter)
{
    painter->setRenderHint(QPainter::Antialiasing, true);

    lrcInstance_->renderer()->drawFrame(get_rendererId(), [this, painter](QImage* previewImage) {
        if (previewImage) {
            QImage scaledPreview;
#if defined(Q_OS_MACOS)

            scaledPreview = Utils::getCirclePhoto(*previewImage,
                                                  height() <= width() ? height() : width())
                                .rgbSwapped();
#else
                scaledPreview = Utils::getCirclePhoto(*previewImage,
                                                      height() <= width() ? height() : width());
#endif
            painter->drawImage(QRect(0, 0, scaledPreview.width(), scaledPreview.height()),
                               scaledPreview);
        }
    });
}
