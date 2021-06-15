/*
 * Copyright (C) 2021 by Savoir-faire Linux
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "globaltestenvironment.h"

TestEnvironment globalEnv;

/*!
 * Test fixture for AccountAdapter testing
 */
class ContactFixture : public ::testing::Test
{
public:
    // Prepare unit test context. Called at
    // prior each unit test execution
    void SetUp() override {}

    // Close unit test context. Called
    // after each unit test ending
    void TearDown() override {}
};

/*!
 * WHEN  Add an SIP contact.
 * THEN  ContactAdded signal should be emitted once.
 */
TEST_F(ContactFixture, AddSIPContactTest)
{
    // AccountAdded signal spy
    QSignalSpy accountAddedSpy(&globalEnv.lrcInstance->accountModel(),
                               &lrc::api::NewAccountModel::accountAdded);

    // Create SIP Acc
    globalEnv.accountAdapter->createSIPAccount(QVariantMap());

    if (accountAddedSpy.count() < 1)
        QVERIFY(accountAddedSpy.wait());

    QList<QVariant> accountAddedArguments = accountAddedSpy.takeFirst();
    QVERIFY(accountAddedArguments.at(0).type() == QVariant::String);

    // Select the created account
    globalEnv.lrcInstance->setCurrentAccountId(accountAddedArguments.at(0).toString());

    // Make sure the account setup is done
    QSignalSpy accountStatusChangedSpy(&globalEnv.lrcInstance->accountModel(),
                                       &lrc::api::NewAccountModel::accountStatusChanged);

    if (accountStatusChangedSpy.count() < 1)
        QVERIFY(accountStatusChangedSpy.wait());

    // ModelUpdated signal spy
    QSignalSpy modelUpdatedSpy(globalEnv.lrcInstance->getCurrentContactModel(),
                               &lrc::api::ContactModel::modelUpdated);

    // Add temp contact test
    globalEnv.lrcInstance->getCurrentConversationModel()->setFilter("test");

    if (modelUpdatedSpy.count() < 1)
        QVERIFY(modelUpdatedSpy.wait());

    QList<QVariant> modelUpdatedArguments = modelUpdatedSpy.takeFirst();
    QVERIFY(modelUpdatedArguments.at(0).type() == QVariant::String);

    // Get conversation id
    auto convId = globalEnv.lrcInstance
                      ->getConversationFromPeerUri(modelUpdatedArguments.at(0).toString())
                      .uid;
    ASSERT_EQ(convId.isEmpty(), false);

    // ContactAdded signal spy
    QSignalSpy contactAddedSpy(globalEnv.lrcInstance->getCurrentContactModel(),
                               &lrc::api::ContactModel::contactAdded);

    globalEnv.lrcInstance->getCurrentConversationModel()->makePermanent(convId);

    if (contactAddedSpy.count() < 1)
        QVERIFY(contactAddedSpy.wait());

    // Remove the account
    globalEnv.lrcInstance->accountModel().removeAccount(
        globalEnv.lrcInstance->getCurrentAccountId());

    QSignalSpy accountRemovedSpy(&globalEnv.lrcInstance->accountModel(),
                                 &lrc::api::NewAccountModel::accountRemoved);

    if (accountRemovedSpy.count() < 1)
        QVERIFY(accountRemovedSpy.wait());
}