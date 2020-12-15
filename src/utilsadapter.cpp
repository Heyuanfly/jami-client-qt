/*!
 * Copyright (C) 2015-2020 by Savoir-faire Linux
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

#include "globalsystemtray.h"
#include "lrcinstance.h"
#include "utils.h"
#include "version.h"

#include <QApplication>
#include <QClipboard>
#include <QFileInfo>

UtilsAdapter::UtilsAdapter(QObject* parent)
    : QObject(parent)
    , clipboard_(QApplication::clipboard())
{}

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
UtilsAdapter::setText(QString text)
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
    const auto& conv = LRCInstance::getConversationFromConvUid(uid);
    if (!conv.participants.isEmpty())
        return LRCInstance::getAccountInfo(accountId).contactModel->bestNameForContact(
            conv.participants[0]);
    return QString();
}

QString
UtilsAdapter::getBestId(const QString& accountId)
{
    if (accountId.isEmpty())
        return {};
    return LRCInstance::accountModel().bestIdForAccount(accountId);
}

const QString
UtilsAdapter::getBestId(const QString& accountId, const QString& uid)
{
    const auto& conv = LRCInstance::getConversationFromConvUid(uid);
    if (!conv.participants.isEmpty())
        return LRCInstance::getAccountInfo(accountId).contactModel->bestIdForContact(
            conv.participants[0]);
    return QString();
}

int
UtilsAdapter::getTotalUnreadMessages()
{
    int totalUnreadMessages {0};
    if (LRCInstance::getCurrentAccountInfo().profileInfo.type != lrc::api::profile::Type::SIP) {
        auto* convModel = LRCInstance::getCurrentConversationModel();
        auto ringConversations = convModel->getFilteredConversations(lrc::api::profile::Type::RING,
                                                                     false);
        ringConversations.for_each(
            [&totalUnreadMessages](const lrc::api::conversation::Info& conversation) {
                totalUnreadMessages += conversation.unreadMessages;
            });
    }
    return totalUnreadMessages;
}

int
UtilsAdapter::getTotalPendingRequest()
{
    auto& accountInfo = LRCInstance::getCurrentAccountInfo();
    return accountInfo.contactModel->pendingRequestCount();
}

void
UtilsAdapter::setConversationFilter(const QString& filter)
{
    LRCInstance::getCurrentConversationModel()->setFilter(filter);
}

const QString
UtilsAdapter::getCurrConvId()
{
    return LRCInstance::getCurrentConvUid();
}

void
UtilsAdapter::makePermanentCurrentConv()
{
    LRCInstance::getCurrentConversationModel()->makePermanent(LRCInstance::getCurrentConvUid());
}

const QStringList
UtilsAdapter::getCurrAccList()
{
    return LRCInstance::accountModel().getAccountList();
}

int
UtilsAdapter::getAccountListSize()
{
    return getCurrAccList().size();
}

void
UtilsAdapter::setCurrentCall(const QString& accountId, const QString& convUid)
{
    auto& accInfo = LRCInstance::getAccountInfo(accountId);
    auto const& convInfo = LRCInstance::getConversationFromConvUid(convUid, accountId);
    accInfo.callModel->setCurrentCall(convInfo.callId);
}

bool
UtilsAdapter::hasCall(const QString& accountId)
{
    auto activeCalls = LRCInstance::getActiveCalls();
    for (const auto& callId : activeCalls) {
        auto& accountInfo = LRCInstance::accountModel().getAccountInfo(accountId);
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
    for (const auto& callId : LRCInstance::getActiveCalls()) {
        auto& accountInfo = LRCInstance::accountModel().getAccountInfo(accountId);
        if (accountInfo.callModel->hasCall(callId)) {
            return LRCInstance::getConversationFromCallId(callId, accountId).uid;
        }
    }
    return "";
}

const QString
UtilsAdapter::getCallId(const QString& accountId, const QString& convUid)
{
    auto const& convInfo = LRCInstance::getConversationFromConvUid(convUid, accountId);
    if (convInfo.uid.isEmpty()) {
        return {};
    }

    if (auto* call = LRCInstance::getCallInfoForConversation(convInfo, false)) {
        return call->id;
    }

    return {};
}

int
UtilsAdapter::getCallStatus(const QString& callId)
{
    const auto callStatus = LRCInstance::getCallInfo(callId, LRCInstance::getCurrAccId());
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
    return value ? QString::number(static_cast<float>(value) / 100, 'f', 1) + " Mbps" : "Default";
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
    return path.replace(QRegExp("^file:\\/{2,3}"), "").replace("\n", "").replace("\r", "");
#else
    return path.replace(QRegExp("^file:\\/{2,3}"), "/").replace("\n", "").replace("\r", "");
#endif
}

bool
UtilsAdapter::checkShowPluginsButton()
{
    return LRCInstance::pluginModel().getPluginsEnabled()
           && (LRCInstance::pluginModel().listLoadedPlugins().size() > 0);
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
    GlobalSystemTray::instance().setVisible(visible);
}
