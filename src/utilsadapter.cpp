/*!
 * Copyright (C) 2015-2022 Savoir-faire Linux Inc.
 * Author: Edric Ladent Milaret <edric.ladent-milaret@savoirfairelinux.com>
 * Author: Andreas Traczyk <andreas.traczyk@savoirfairelinux.com>
 * Author: Isa Nanic <isa.nanic@savoirfairelinux.com>
 * Author: Mingrui Zhang   <mingrui.zhang@savoirfairelinux.com>
 * Author: Aline Gondim Santos   <aline.gondimsantos@savoirfairelinux.com>
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

#include "utilsadapter.h"

#include "lrcinstance.h"
#include "systemtray.h"
#include "utils.h"
#include "version.h"

#include "api/pluginmodel.h"
#include "api/datatransfermodel.h"

#include <QApplication>
#include <QBuffer>
#include <QClipboard>
#include <QFileInfo>
#include <QRegExp>

UtilsAdapter::UtilsAdapter(AppSettingsManager* settingsManager,
                           SystemTray* systemTray,
                           LRCInstance* instance,
                           QObject* parent)
    : QmlAdapterBase(instance, parent)
    , clipboard_(QApplication::clipboard())
    , systemTray_(systemTray)
    , settingsManager_(settingsManager)
{
    if (lrcInstance_->avModel().getRecordPath().isEmpty()) {
        lrcInstance_->avModel().setRecordPath(getDefaultRecordPath());
    }
}

const QString
UtilsAdapter::getProjectCredits()
{
    return Utils::getProjectCredits();
}

const QString
UtilsAdapter::getVersionStr()
{
    return QString(VERSION_STRING);
}

void
UtilsAdapter::setClipboardText(QString text)
{
    clipboard_->setText(text, QClipboard::Clipboard);
}

const QString
UtilsAdapter::qStringFromFile(const QString& filename)
{
    return Utils::QByteArrayFromFile(filename);
}

const QString
UtilsAdapter::getStyleSheet(const QString& name, const QString& source)
{
    auto simplifiedCSS = source.simplified().replace("'", "\"");
    QString s = QString::fromLatin1("(function() {"
                                    "    var node = document.createElement('style');"
                                    "    node.id = '%1';"
                                    "    node.innerHTML = '%2';"
                                    "    document.head.appendChild(node);"
                                    "})()")
                    .arg(name)
                    .arg(simplifiedCSS);
    return s;
}

const QString
UtilsAdapter::getCachePath()
{
    QDir dataDir(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation));
    dataDir.cdUp();
    return dataDir.absolutePath() + "/jami";
}

QString
UtilsAdapter::getDefaultRecordPath() const
{
    auto defaultDirectory = QStandardPaths::writableLocation(QStandardPaths::MoviesLocation)
                            + "/Jami";
    QDir dir(defaultDirectory);
    if (!dir.exists())
        dir.mkpath(".");
    return defaultDirectory;
}

bool
UtilsAdapter::createStartupLink()
{
    return Utils::CreateStartupLink(L"Jami");
}

QString
UtilsAdapter::GetRingtonePath()
{
    return Utils::GetRingtonePath();
}

bool
UtilsAdapter::checkStartupLink()
{
    return Utils::CheckStartupLink(L"Jami");
}

const QString
UtilsAdapter::getBestName(const QString& accountId, const QString& uid)
{
    const auto& conv = lrcInstance_->getConversationFromConvUid(uid);
    if (!conv.participants.isEmpty())
        return lrcInstance_->getAccountInfo(accountId).contactModel->bestNameForContact(
            conv.participants[0].uri);
    return QString();
}

QString
UtilsAdapter::getBestNameForUri(const QString& accountId, const QString& uri)
{
    return lrcInstance_->getAccountInfo(accountId).contactModel->bestNameForContact(uri);
}

const QString
UtilsAdapter::getPeerUri(const QString& accountId, const QString& uid)
{
    try {
        auto* convModel = lrcInstance_->getAccountInfo(accountId).conversationModel.get();
        const auto& convInfo = convModel->getConversationForUid(uid).value();
        return convInfo.get().participants.front().uri;
    } catch (const std::out_of_range& e) {
        qDebug() << e.what();
        return "";
    }
}

QString
UtilsAdapter::getBestId(const QString& accountId)
{
    if (accountId.isEmpty())
        return {};
    return lrcInstance_->accountModel().bestIdForAccount(accountId);
}

const QString
UtilsAdapter::getBestId(const QString& accountId, const QString& uid)
{
    const auto& conv = lrcInstance_->getConversationFromConvUid(uid);
    if (!conv.participants.isEmpty())
        return lrcInstance_->getAccountInfo(accountId).contactModel->bestIdForContact(
            conv.participants[0].uri);
    return QString();
}

void
UtilsAdapter::setConversationFilter(const QString& filter)
{
    lrcInstance_->getCurrentConversationModel()->setFilter(filter);
}

const QStringList
UtilsAdapter::getCurrAccList()
{
    return lrcInstance_->accountModel().getAccountList();
}

int
UtilsAdapter::getAccountListSize()
{
    return getCurrAccList().size();
}

bool
UtilsAdapter::hasCall(const QString& accountId)
{
    auto activeCalls = lrcInstance_->getActiveCalls();
    for (const auto& callId : activeCalls) {
        auto& accountInfo = lrcInstance_->accountModel().getAccountInfo(accountId);
        if (accountInfo.callModel->hasCall(callId)) {
            return true;
        }
    }
    return false;
}

const QString
UtilsAdapter::getCallConvForAccount(const QString& accountId)
{
    // TODO: Currently returning first call, establish priority according to state?
    for (const auto& callId : lrcInstance_->getActiveCalls()) {
        auto& accountInfo = lrcInstance_->accountModel().getAccountInfo(accountId);
        if (accountInfo.callModel->hasCall(callId)) {
            return lrcInstance_->getConversationFromCallId(callId, accountId).uid;
        }
    }
    return "";
}

const QString
UtilsAdapter::getCallId(const QString& accountId, const QString& convUid)
{
    auto const& convInfo = lrcInstance_->getConversationFromConvUid(convUid, accountId);
    if (convInfo.uid.isEmpty()) {
        return {};
    }

    if (auto* call = lrcInstance_->getCallInfoForConversation(convInfo, false)) {
        return call->id;
    }

    return {};
}

int
UtilsAdapter::getCallStatus(const QString& callId)
{
    const auto callStatus = lrcInstance_->getCallInfo(callId, lrcInstance_->get_currentAccountId());
    return static_cast<int>(callStatus->status);
}

const QString
UtilsAdapter::getCallStatusStr(int statusInt)
{
    const auto status = static_cast<lrc::api::call::Status>(statusInt);
    return lrc::api::call::to_string(status);
}

// returns true if name is valid registered name
bool
UtilsAdapter::validateRegNameForm(const QString& regName)
{
    QRegularExpression regExp(" ");

    if (regName.size() > 2 && !regName.contains(regExp)) {
        return true;

    } else {
        return false;
    }
}

QString
UtilsAdapter::getStringUTF8(QString string)
{
    return string.toUtf8();
}

QString
UtilsAdapter::getRecordQualityString(int value)
{
    auto valueStr = QString::number(static_cast<float>(value) / 100, 'f', 1);
    return value ? tr("%1 Mbps").arg(valueStr) : tr("Default");
}

QString
UtilsAdapter::getCurrentPath()
{
    return QDir::currentPath();
}

QString
UtilsAdapter::stringSimplifier(QString input)
{
    return input.simplified();
}

QString
UtilsAdapter::toNativeSeparators(QString inputDir)
{
    return QDir::toNativeSeparators(inputDir);
}

QString
UtilsAdapter::toFileInfoName(QString inputFileName)
{
    QFileInfo fi(inputFileName);
    return fi.fileName();
}

QString
UtilsAdapter::toFileAbsolutepath(QString inputFileName)
{
    QFileInfo fi(inputFileName);
    return fi.absolutePath();
}

QString
UtilsAdapter::getAbsPath(QString path)
{
    // Note: this function is used on urls returned from qml-FileDialogs which
    // contain 'file:///' for reasons we don't understand.
    // TODO: this logic can be refactored into the JamiFileDialog component.
#ifdef Q_OS_WIN
    return path.replace(QRegularExpression("^file:\\/{2,3}"), "").replace("\n", "").replace("\r", "");
#else
    return path.replace(QRegularExpression("^file:\\/{2,3}"), "/")
        .replace("\n", "")
        .replace("\r", "");
#endif
}

QString
UtilsAdapter::fileName(const QString& path)
{
    QFileInfo fi(path);
    return fi.fileName();
}

QString
UtilsAdapter::getExt(const QString& path)
{
    QFileInfo fi(path);
    return fi.completeSuffix();
}

bool
UtilsAdapter::isImage(const QString& fileExt)
{
    return Utils::isImage(fileExt);
}

QString
UtilsAdapter::humanFileSize(qint64 fileSize)
{
    return Utils::humanFileSize(fileSize);
}

void
UtilsAdapter::setSystemTrayIconVisible(bool visible)
{
    systemTray_->setVisible(visible);
}

QVariant
UtilsAdapter::getAppValue(const Settings::Key key)
{
    return settingsManager_->getValue(key);
}

void
UtilsAdapter::setAppValue(const Settings::Key key, const QVariant& value)
{
    settingsManager_->setValue(key, value);
    // If we change the lang preference, reload the translations
    if (key == Settings::Key::LANG)
        settingsManager_->loadTranslations();
    else if (key == Settings::Key::EnableExperimentalSwarm)
        Q_EMIT showExperimentalSwarm();
}

QString
UtilsAdapter::getDirDocument()
{
    return QDir::toNativeSeparators(
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation));
}

QString
UtilsAdapter::getDirDownload()
{
    QString downloadPath = QDir::toNativeSeparators(lrcInstance_->accountModel().downloadDirectory);
    if (downloadPath.isEmpty()) {
        downloadPath = lrc::api::DataTransferModel::createDefaultDirectory();
        setDownloadPath(downloadPath);
        lrcInstance_->accountModel().downloadDirectory = downloadPath;
    }
#ifdef Q_OS_WIN
    int pos = downloadPath.lastIndexOf(QChar('\\'));
#else
    int pos = downloadPath.lastIndexOf(QChar('/'));
#endif
    if (pos == downloadPath.length() - 1)
        downloadPath.truncate(pos);
    return downloadPath;
}

void
UtilsAdapter::setRunOnStartUp(bool state)
{
    if (Utils::CheckStartupLink(L"Jami")) {
        if (!state) {
            Utils::DeleteStartupLink(L"Jami");
        }
    } else if (state) {
        Utils::CreateStartupLink(L"Jami");
    }
}

void
UtilsAdapter::setDownloadPath(QString dir)
{
    setAppValue(Settings::Key::DownloadPath, dir);
    lrcInstance_->accountModel().downloadDirectory = dir + "/";
}

void
UtilsAdapter::monitor(const bool& continuous)
{
    disconnect(debugMessageReceivedConnection_);
    if (continuous)
        debugMessageReceivedConnection_
            = QObject::connect(&lrcInstance_->behaviorController(),
                               &lrc::api::BehaviorController::debugMessageReceived,
                               [this](const QString& data) {
                                   logList_.append(data);
                                   if (logList_.size() >= LOGSLIMIT) {
                                       logList_.removeFirst();
                                   }
                                   Q_EMIT debugMessageReceived(data);
                               });
    lrcInstance_->monitor(continuous);
}

void
UtilsAdapter::clearInteractionsCache(const QString& accountId, const QString& convId)
{
    if (lrcInstance_->get_selectedConvUid() != convId) {
        try {
            auto& accInfo = lrcInstance_->accountModel().getAccountInfo(accountId);
            auto& convModel = accInfo.conversationModel;
            convModel->clearInteractionsCache(convId);
        } catch (...) {
        }
    }
}

QVariantMap
UtilsAdapter::supportedLang()
{
#if defined(Q_OS_LINUX) && defined(JAMI_INSTALL_PREFIX)
    QString appDir = JAMI_INSTALL_PREFIX;
#elif defined(Q_OS_MACOS)
    QDir dir(qApp->applicationDirPath());
    dir.cdUp();
    QString appDir = dir.absolutePath() + "/Resources/share";
#else
    QString appDir = qApp->applicationDirPath() + QDir::separator() + "share";
#endif
    auto trDir = QDir(appDir + QDir::separator() + "ring" + QDir::separator() + "translations");
    QStringList trFiles = trDir.entryList(QStringList() << "ring_client_windows_*.qm", QDir::Files);
    QVariantMap result;
    result["SYSTEM"] = tr("System");
    // Get available locales
    QRegExp regex("ring_client_windows_(.*).qm");
    QSet<QString> nativeNames;
    for (const auto& f : trFiles) {
        auto match = regex.indexIn(f);
        if (regex.capturedTexts().size() == 2) {
            const auto& l = regex.capturedTexts()[1];
            auto nativeName = QLocale(l).nativeLanguageName();
            if (nativeName.isEmpty()) // If a locale doesn't have any nativeLanguageName, ignore it.
                continue;
            // Avoid to show potential duplicates.
            if (!nativeNames.contains(nativeName)) {
                result[l] = nativeName;
                nativeNames.insert(nativeName);
            }
        }
    }
    return result;
}

QString
UtilsAdapter::swarmCreationImage(const QString& imageId) const
{
    if (imageId == "temp")
        return Utils::QByteArrayFromFile(
            QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + "tmpSwarmImage");
    return lrcInstance_->getCurrentConversationModel()->avatar(imageId);
}

void
UtilsAdapter::setSwarmCreationImageFromString(const QString& image, const QString& imageId)
{
    // Compress the image before saving
    auto img = Utils::imageFromBase64String(image, false);
    setSwarmCreationImageFromImage(img, imageId);
}

void
UtilsAdapter::setSwarmCreationImageFromFile(const QString& path, const QString& imageId)
{
    // Compress the image before saving
    auto image = Utils::QByteArrayFromFile(path);
    auto img = Utils::imageFromBase64Data(image, false);
    setSwarmCreationImageFromImage(img, imageId);
}

void
UtilsAdapter::setSwarmCreationImageFromImage(const QImage& image, const QString& imageId)
{
    // Compress the image before saving
    auto img = Utils::scaleAndFrame(image, QSize(256, 256));
    QByteArray ba;
    QBuffer bu(&ba);
    img.save(&bu, "PNG");
    // Save the image
    if (imageId == "temp") {
        QFile file(QStandardPaths::writableLocation(QStandardPaths::CacheLocation)
                   + "tmpSwarmImage");
        file.open(QIODevice::WriteOnly);
        file.write(ba.toBase64());
        file.close();
        Q_EMIT lrcInstance_->base64SwarmAvatarChanged();
    } else {
        lrcInstance_->getCurrentConversationModel()->updateConversationInfos(imageId,
                                                                             {{"avatar",
                                                                               ba.toBase64()}});
    }
}

bool
UtilsAdapter::getContactPresence(const QString& accountId, const QString& uri)
{
    try {
        if (lrcInstance_->getAccountInfo(accountId).profileInfo.uri == uri)
            return true; // It's the same account
        auto info = lrcInstance_->getAccountInfo(accountId).contactModel->getContact(uri);
        return info.isPresent;
    } catch (...) {
    }
    return false;
}

QString
UtilsAdapter::getContactBestName(const QString& accountId, const QString& uri)
{
    try {
        if (lrcInstance_->getAccountInfo(accountId).profileInfo.uri == uri)
            return lrcInstance_->accountModel().bestNameForAccount(
                accountId); // It's the same account
        return lrcInstance_->getAccountInfo(accountId).contactModel->bestNameForContact(uri);
    } catch (...) {
    }
    return {};
}

lrc::api::member::Role
UtilsAdapter::getParticipantRole(const QString& accountId, const QString& convId, const QString& uri)
{
    try {
        return lrcInstance_->getAccountInfo(accountId).conversationModel->memberRole(convId, uri);
    } catch (...) {
    }
    return lrc::api::member::Role::MEMBER;
}

bool
UtilsAdapter::luma(const QColor& color) const
{
    return (0.2126 * color.red() + 0.7152 * color.green() + 0.0722 * color.blue()) > .6;
}