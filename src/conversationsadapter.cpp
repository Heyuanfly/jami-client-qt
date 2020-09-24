/*!
 * Copyright (C) 2020 by Savoir-faire Linux
 * Author: Edric Ladent Milaret <edric.ladent-milaret@savoirfairelinux.com>
 * Author: Anthony L�onard <anthony.leonard@savoirfairelinux.com>
 * Author: Olivier Soldano <olivier.soldano@savoirfairelinux.com>
 * Author: Andreas Traczyk <andreas.traczyk@savoirfairelinux.com>
 * Author: Isa Nanic <isa.nanic@savoirfairelinux.com>
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

#include "conversationsadapter.h"

#include "utils.h"
#include "qtutils.h"

#include <QApplication>

ConversationsAdapter::ConversationsAdapter(QObject* parent)
    : QmlAdapterBase(parent)
{}

void
ConversationsAdapter::safeInit()
{
    conversationSmartListModel_ = new SmartListModel(this, LRCInstance::getCurrAccId());

    emit modelChanged(QVariant::fromValue(conversationSmartListModel_));

    connect(&LRCInstance::behaviorController(),
            &BehaviorController::showChatView,
            [this](const QString& accountId, lrc::api::conversation::Info convInfo) {
                emit showChatView(accountId, convInfo.uid);
            });

    connect(&LRCInstance::behaviorController(),
            &BehaviorController::newUnreadInteraction,
            this,
            &ConversationsAdapter::onNewUnreadInteraction);

    connect(&LRCInstance::instance(),
            &LRCInstance::currentAccountChanged,
            this,
            &ConversationsAdapter::onCurrentAccountIdChanged);

    connectConversationModel();
}

void
ConversationsAdapter::backToWelcomePage()
{
    deselectConversation();
    emit navigateToWelcomePageRequested();
}

void
ConversationsAdapter::selectConversation(const QString& accountId,
                                         const QString& convUid,
                                         bool preventSendingSignal)
{
    auto& accInfo = LRCInstance::getAccountInfo(accountId);
    const auto convInfo = accInfo.conversationModel->getConversationForUID(convUid);

    selectConversation(convInfo, preventSendingSignal);
}

void
ConversationsAdapter::selectConversation(const QString& convUid)
{
    auto* convModel = LRCInstance::getCurrentConversationModel();
    if (convModel == nullptr) {
        return;
    }

    const auto& conversation = convModel->getConversationForUID(convUid);

    if (selectConversation(conversation, false)) {
        // If it is calling, show callview (can use showChatView signal, since it will be determined on qml).
        if (!conversation.uid.isEmpty()
            && LRCInstance::getCurrentCallModel()->hasCall(conversation.callId)) {
            emit showChatView(LRCInstance::getCurrAccId(), conversation.uid);
        }
    }
}

bool
ConversationsAdapter::selectConversation(const lrc::api::conversation::Info& convInfo,
                                         bool preventSendingSignal)
{
    // accInfo.conversationModel->selectConversation(item.uid) only emit ui
    // behavior control signals, but sometimes we do not want that,
    // preventSendingSignal boolean can help us to determine.
    if (LRCInstance::getCurrentConvUid() == convInfo.uid
        && LRCInstance::getCurrAccId() == convInfo.accountId) {
        return false;
    } else if (convInfo.participants.size() > 0) {
        // If the account is not currently selected, do that first, then
        // proceed to select the conversation.
        auto selectConversation = [convInfo, preventSendingSignal] {
            auto& accInfo = LRCInstance::getAccountInfo(convInfo.accountId);
            LRCInstance::setSelectedConvId(convInfo.uid);
            if (!preventSendingSignal)
                accInfo.conversationModel->selectConversation(convInfo.uid);
            accInfo.conversationModel->clearUnreadInteractions(convInfo.uid);
        };
        if (convInfo.accountId != LRCInstance::getCurrAccId()) {
            Utils::oneShotConnect(&LRCInstance::instance(),
                                  &LRCInstance::currentAccountChanged,
                                  [selectConversation] { selectConversation(); });
            LRCInstance::setSelectedAccountId(convInfo.accountId);
        } else {
            selectConversation();
        }
        return true;
    }
}

void
ConversationsAdapter::deselectConversation()
{
    if (LRCInstance::getCurrentConvUid().isEmpty()) {
        return;
    }

    auto currentConversationModel = LRCInstance::getCurrentConversationModel();

    if (currentConversationModel == nullptr) {
        return;
    }

    LRCInstance::setSelectedConvId();
}

void
ConversationsAdapter::onCurrentAccountIdChanged()
{
    auto accountId = LRCInstance::getCurrAccId();

    auto& accountInfo = LRCInstance::accountModel().getAccountInfo(accountId);
    currentTypeFilter_ = accountInfo.profileInfo.type;
    LRCInstance::getCurrentConversationModel()->setFilter(accountInfo.profileInfo.type);
    updateConversationsFilterWidget();

    disconnectConversationModel();
    connectConversationModel();
}

void
ConversationsAdapter::onNewUnreadInteraction(const QString& accountId,
                                             const QString& convUid,
                                             uint64_t interactionId,
                                             const interaction::Info& interaction)
{
    Q_UNUSED(interactionId)
    if (!interaction.authorUri.isEmpty()
        && (!QApplication::focusWindow() || accountId != LRCInstance::getCurrAccId()
            || convUid != LRCInstance::getCurrentConvUid())) {
        auto& accInfo = LRCInstance::getAccountInfo(accountId);
        auto& contact = accInfo.contactModel->getContact(interaction.authorUri);
        auto from = Utils::bestNameForContact(contact);
        auto onClicked = [this, accountId, convUid, uri = interaction.authorUri] {
#ifdef Q_OS_WINDOWS
            emit LRCInstance::instance().notificationClicked();
#else
            emit LRCInstance::instance().notificationClicked(true);
#endif
            auto convInfo = LRCInstance::getConversationFromConvUid(convUid, accountId);
            if (!convInfo.uid.isEmpty()) {
                selectConversation(convInfo, false);
                emit LRCInstance::instance().updateSmartList();
                emit modelSorted(uri);
            }
        };

        Utils::showNotification(interaction.body, from, accountId, convUid, onClicked);
        return;
    }
}

void
ConversationsAdapter::updateConversationsFilterWidget()
{
    // Update status of "Conversations" and "Invitations".
    auto invites = LRCInstance::getCurrentAccountInfo().contactModel->pendingRequestCount();
    if (invites == 0 && currentTypeFilter_ == lrc::api::profile::Type::PENDING) {
        currentTypeFilter_ = lrc::api::profile::Type::RING;
        LRCInstance::getCurrentConversationModel()->setFilter(currentTypeFilter_);
    }
    showConversationTabs(invites);
}

void
ConversationsAdapter::setConversationFilter(const QString& type)
{
    // Set conversation filter according to type,
    // type needs to be recognizable by lrc::api::profile::to_type.
    if (type.isEmpty()) {
        if (LRCInstance::getCurrentAccountInfo().profileInfo.type == lrc::api::profile::Type::RING)
            setConversationFilter(lrc::api::profile::Type::RING);
        else
            setConversationFilter(lrc::api::profile::Type::SIP);
    } else {
        setConversationFilter(lrc::api::profile::to_type(type));
    }
}

void
ConversationsAdapter::setConversationFilter(lrc::api::profile::Type filter)
{
    if (currentTypeFilter_ == filter) {
        return;
    }
    currentTypeFilter_ = filter;
    LRCInstance::getCurrentConversationModel()->setFilter(currentTypeFilter_);
}

void
ConversationsAdapter::refill()
{
    if (conversationSmartListModel_)
        conversationSmartListModel_->fillConversationsList();
}

bool
ConversationsAdapter::connectConversationModel(bool updateFilter)
{
    // Signal connections
    auto currentConversationModel = LRCInstance::getCurrentConversationModel();

    modelSortedConnection_ = QObject::connect(
        currentConversationModel, &lrc::api::ConversationModel::modelSorted, [this]() {
            conversationSmartListModel_->fillConversationsList();
            updateConversationsFilterWidget();
            emit updateListViewRequested();
            auto* convModel = LRCInstance::getCurrentConversationModel();
            const auto conversation = convModel->getConversationForUID(
                LRCInstance::getCurrentConvUid());

            if (conversation.uid.isEmpty() || conversation.participants.isEmpty()) {
                return;
            }
            const auto contactURI = conversation.participants[0];
            if (contactURI.isEmpty()
                || convModel->owner.contactModel->getContact(contactURI).profileInfo.type
                       == lrc::api::profile::Type::TEMPORARY) {
                return;
            }
            emit modelSorted(QVariant::fromValue(contactURI));
        });

    modelUpdatedConnection_ = QObject::connect(currentConversationModel,
                                               &lrc::api::ConversationModel::conversationUpdated,
                                               [this](const QString& convUid) {
                                                   conversationSmartListModel_->updateConversation(
                                                       convUid);
                                                   updateConversationsFilterWidget();
                                                   emit updateListViewRequested();
                                               });

    filterChangedConnection_
        = QObject::connect(currentConversationModel,
                           &lrc::api::ConversationModel::filterChanged,
                           [this]() {
                               conversationSmartListModel_->fillConversationsList();
                               updateConversationsFilterWidget();
                               emit updateListViewRequested();
                           });

    newConversationConnection_
        = QObject::connect(currentConversationModel,
                           &lrc::api::ConversationModel::newConversation,
                           [this](const QString& convUid) {
                               conversationSmartListModel_->fillConversationsList();
                               updateConversationForNewContact(convUid);
                           });

    conversationRemovedConnection_
        = QObject::connect(currentConversationModel,
                           &lrc::api::ConversationModel::conversationRemoved,
                           [this]() {
                               conversationSmartListModel_->fillConversationsList();
                               backToWelcomePage();
                           });

    conversationClearedConnection
        = QObject::connect(currentConversationModel,
                           &lrc::api::ConversationModel::conversationCleared,
                           [this](const QString& convUid) {
                               // If currently selected, switch to welcome screen (deselecting
                               // current smartlist item ).
                               if (convUid != LRCInstance::getCurrentConvUid()) {
                                   return;
                               }
                               backToWelcomePage();
                           });

    searchStatusChangedConnection_
        = QObject::connect(currentConversationModel,
                           &lrc::api::ConversationModel::searchStatusChanged,
                           [this](const QString& status) { emit showSearchStatus(status); });

    searchResultUpdatedConnection_
        = QObject::connect(currentConversationModel,
                           &lrc::api::ConversationModel::searchResultUpdated,
                           [this]() {
                               conversationSmartListModel_->fillConversationsList();
                               emit updateListViewRequested();
                           });

    if (updateFilter)
        currentConversationModel->setFilter("");
    return true;
}

void
ConversationsAdapter::disconnectConversationModel()
{
    QObject::disconnect(modelSortedConnection_);
    QObject::disconnect(modelUpdatedConnection_);
    QObject::disconnect(filterChangedConnection_);
    QObject::disconnect(newConversationConnection_);
    QObject::disconnect(conversationRemovedConnection_);
    QObject::disconnect(conversationClearedConnection);
    QObject::disconnect(selectedCallChanged_);
    QObject::disconnect(smartlistSelectionConnection_);
    QObject::disconnect(interactionRemovedConnection_);
    QObject::disconnect(searchStatusChangedConnection_);
    QObject::disconnect(searchResultUpdatedConnection_);
}

void
ConversationsAdapter::updateConversationForNewContact(const QString& convUid)
{
    auto* convModel = LRCInstance::getCurrentConversationModel();
    if (convModel == nullptr) {
        return;
    }
    const auto selectedUid = LRCInstance::getCurrentConvUid();
    const auto conversation = convModel->getConversationForUID(convUid);
    if (!conversation.uid.isEmpty() && !conversation.participants.isEmpty()) {
        try {
            const auto contact = convModel->owner.contactModel->getContact(
                conversation.participants[0]);
            if (!contact.profileInfo.uri.isEmpty() && contact.profileInfo.uri == selectedUid) {
                LRCInstance::setSelectedConvId(convUid);
                convModel->selectConversation(convUid);
            }
        } catch (...) {
            return;
        }
    }
}
