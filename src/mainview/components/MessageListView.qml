/*
 * Copyright (C) 2021-2022 Savoir-faire Linux Inc.
 * Author: Trevor Tabah <trevor.tabah@savoirfairelinux.com>
 * Author: Andreas Traczyk <andreas.traczyk@savoirfairelinux.com>
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

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt.labs.qmlmodels

import net.jami.Models 1.1
import net.jami.Adapters 1.1
import net.jami.Constants 1.1

import "../../commoncomponents"

JamiListView {
    id: root

    function getDistanceToBottom() {
        const scrollDiff = ScrollBar.vertical.position -
                         (1.0 - ScrollBar.vertical.size)
        return Math.abs(scrollDiff) * contentHeight
    }

    function loadMoreMsgsIfNeeded() {
        if (atYBeginning && !CurrentConversation.allMessagesLoaded)
            MessagesAdapter.loadMoreMessages()
    }

    // sequencing/timestamps (2-sided style)
    function computeTimestampVisibility(item, itemIndex) {
        if (root === undefined)
            return
        var nItem = root.itemAtIndex(itemIndex - 1)
        if (nItem && itemIndex !== root.count - 1) {
            item.showTime = (nItem.timestamp - item.timestamp) > 60 &&
                    nItem.formattedTime !== item.formattedTime
        } else {
            item.showTime = true
            var pItem = root.itemAtIndex(itemIndex + 1)
            if (pItem) {
                pItem.showTime = (item.timestamp - pItem.timestamp) > 60 &&
                        pItem.formattedTime !== item.formattedTime
            }
        }
    }

    function computeSequencing(computeItem, computeItemIndex) {
        if (root === undefined)
            return
        var cItem = {
            'author': computeItem.author,
            'showTime': computeItem.showTime
        }
        var pItem = root.itemAtIndex(computeItemIndex + 1)
        var nItem = root.itemAtIndex(computeItemIndex - 1)

        let isSeq = (item0, item1) =>
            item0.author === item1.author && !item0.showTime

        let setSeq = function (newSeq, item) {
            if (item === undefined)
                computeItem.seq = newSeq
            else
                item.seq = newSeq
        }

        let rAdjustSeq = function (item) {
            if (item.seq === MsgSeq.last)
                item.seq = MsgSeq.middle
            else if (item.seq === MsgSeq.single)
                setSeq(MsgSeq.first, item)
        }

        let adjustSeq = function (item) {
            if (item.seq === MsgSeq.first)
                item.seq = MsgSeq.middle
            else if (item.seq === MsgSeq.single)
                setSeq(MsgSeq.last, item)
        }

        if (pItem && !nItem) {
            if (!isSeq(pItem, cItem)) {
                computeItem.seq = MsgSeq.single
            } else {
                computeItem.seq = MsgSeq.last
                rAdjustSeq(pItem)
            }
        } else if (nItem && !pItem) {
            if (!isSeq(cItem, nItem)) {
                computeItem.seq = MsgSeq.single
            } else {
                setSeq(MsgSeq.first)
                adjustSeq(nItem)
            }
        } else if (!nItem && !pItem) {
            computeItem.seq = MsgSeq.single
        } else {
            if (isSeq(pItem, nItem)) {
                if (isSeq(pItem, cItem)) {
                    computeItem.seq = MsgSeq.middle
                } else {
                    computeItem.seq = MsgSeq.single

                    if (pItem.seq === MsgSeq.first)
                        pItem.seq = MsgSeq.single
                    else if (item.seq === MsgSeq.middle)
                        pItem.seq = MsgSeq.last

                    if (nItem.seq === MsgSeq.last)
                        nItem.seq = MsgSeq.single
                    else if (nItem.seq === MsgSeq.middle)
                        nItem.seq = MsgSeq.first
                }
            } else {
                if (!isSeq(pItem, cItem)) {
                    computeItem.seq = MsgSeq.first
                    adjustSeq(pItem)
                } else {
                    computeItem.seq = MsgSeq.last
                    rAdjustSeq(nItem)
                }
            }
        }

        if (computeItem.seq === MsgSeq.last) {
            computeItem.showTime = true
        }
    }

    // fade-in mechanism
    Component.onCompleted: fadeAnimation.start()
    Rectangle {
        id: overlay
        anchors.fill: parent
        color: JamiTheme.chatviewBgColor
        visible: opacity !== 0
        SequentialAnimation {
            id: fadeAnimation
            NumberAnimation {
                target: overlay; property: "opacity"
                to: 1; duration: 0
            }
            NumberAnimation {
                target: overlay; property: "opacity"
                to: 0; duration: 240
            }
        }
    }
    Connections {
        target: CurrentConversation
        function onIdChanged() { fadeAnimation.start() }
    }

    topMargin: 12
    spacing: 2
    anchors.centerIn: parent
    height: parent.height
    width: parent.width
    // this offscreen caching is pretty huge
    // displayMarginEnd may be removed
    displayMarginBeginning: 2048
    displayMarginEnd: 2048
    maximumFlickVelocity: 2048
    verticalLayoutDirection: ListView.BottomToTop
    boundsBehavior: Flickable.StopAtBounds
    currentIndex: -1

    model: MessagesAdapter.messageListModel

    delegate: DelegateChooser {
        id: delegateChooser

        role: "Type"
        DelegateChoice {
            roleValue: Interaction.Type.TEXT
            TextMessageDelegate {
                Component.onCompleted: {
                    if (index) {
                        computeTimestampVisibility(this, index)
                        computeSequencing(this, index)
                    } else {
                        Qt.callLater(computeTimestampVisibility, this, index)
                        Qt.callLater(computeSequencing, this, index)
                    }
                }
            }
        }
        DelegateChoice {
            roleValue: Interaction.Type.CALL
            GeneratedMessageDelegate {
                Component.onCompleted: {
                    if (index)
                        computeTimestampVisibility(this, index)
                    else
                        Qt.callLater(computeTimestampVisibility, this, index)
                }
            }
        }
        DelegateChoice {
            roleValue: Interaction.Type.CONTACT
            ContactMessageDelegate {
                Component.onCompleted: {
                    if (index)
                        computeTimestampVisibility(this, index)
                    else
                        Qt.callLater(computeTimestampVisibility, this, index)
                }
            }
        }
        DelegateChoice {
            roleValue: Interaction.Type.INITIAL
            GeneratedMessageDelegate {
                font.bold: true
                Component.onCompleted: {
                    if (index)
                        computeTimestampVisibility(this, index)
                    else
                        Qt.callLater(computeTimestampVisibility, this, index)
                }
            }
        }
        DelegateChoice {
            roleValue: Interaction.Type.DATA_TRANSFER
            DataTransferMessageDelegate {
                Component.onCompleted: {
                    if (index) {
                        computeTimestampVisibility(this, index)
                        computeSequencing(this, index)
                    } else {
                        Qt.callLater(computeTimestampVisibility, this, index)
                        Qt.callLater(computeSequencing, this, index)
                    }
                }
            }
        }
    }

    onAtYBeginningChanged: loadMoreMsgsIfNeeded()

    Connections {
        target: MessagesAdapter

        function onNewInteraction() {
            if (root.getDistanceToBottom() < 80 &&
                    !root.atYEnd) {
                Qt.callLater(root.positionViewAtBeginning)
            }
        }

        function onMoreMessagesLoaded() {
            if (root.contentHeight < root.height) {
                root.loadMoreMsgsIfNeeded()
            }
        }
    }

    ScrollToBottomButton {
        id: scrollToBottomButton

        anchors.bottom: root.bottom
        anchors.bottomMargin: JamiTheme.chatViewScrollToBottomButtonBottomMargin
        anchors.horizontalCenter: root.horizontalCenter

        activeStateTrigger: Math.abs(root.contentY) > root.height * 2
        onClicked: root.ScrollBar.vertical.position =
                   1.0 - root.ScrollBar.vertical.size
    }

    header: Control {
        id: typeIndicatorContainer

        topPadding: 6

        width: root.width
        height: typeIndicatorNameText.contentHeight + topPadding

        visible: MessagesAdapter.currentConvComposingList.length

        RowLayout {
            anchors.left: typeIndicatorContainer.left
            anchors.leftMargin: JamiTheme.messageBarMarginSize
            anchors.bottom: typeIndicatorContainer.bottom
            anchors.bottomMargin: 2

            spacing: 0

            TypingDots {
                id: typingDots

                Layout.alignment: Qt.AlignVCenter
            }

            Connections {
                target: MessagesAdapter

                function onCurrentConvComposingListChanged () {
                    var typeIndicatorNameTextString = ""
                    var nameList = MessagesAdapter.currentConvComposingList

                    if (nameList.length > 4) {
                        typeIndicatorNameText.text = ""
                        typeIndicatorEndingText.text = JamiStrings.typeIndicatorMax
                        typeIndicatorNameText.calculateWidth()
                        return
                    }
                    if (nameList.length === 1) {
                        typeIndicatorNameText.text = nameList[0]
                        typeIndicatorEndingText.text =
                                JamiStrings.typeIndicatorSingle.replace("{}", "")
                        typeIndicatorNameText.calculateWidth()
                        return
                    }

                    for (var i = 0; i < nameList.length; i++) {
                        typeIndicatorNameTextString += nameList[i]

                        if (i === nameList.length - 2)
                            typeIndicatorNameTextString += JamiStrings.typeIndicatorAnd
                        else if (i !== nameList.length - 1)
                            typeIndicatorNameTextString += ", "
                    }
                    typeIndicatorNameText.text = typeIndicatorNameTextString
                    typeIndicatorEndingText.text =
                            JamiStrings.typeIndicatorPlural.replace("{}", "")
                    typeIndicatorNameText.calculateWidth()
                }
            }

            Text {
                id: typeIndicatorNameText

                property int textWidth: 0

                function calculateWidth () {
                    if (!text)
                        return 0
                    else {
                        var textSize = JamiQmlUtils.getTextBoundingRect(font, text).width
                        var typingContentWidth = typingDots.width + typingDots.anchors.leftMargin
                                               + typeIndicatorNameText.anchors.leftMargin
                                               + typeIndicatorEndingText.contentWidth
                        typeIndicatorNameText.Layout.preferredWidth =
                                Math.min(typeIndicatorContainer.width - 5 - typingContentWidth,
                                         textSize)
                    }
                }

                Layout.alignment: Qt.AlignVCenter
                Layout.leftMargin: JamiTheme.sbsMessageBasePreferredPadding

                font.pointSize: 8
                font.bold: Font.DemiBold
                elide: Text.ElideRight
                color: JamiTheme.textColor
            }

            Text {
                id: typeIndicatorEndingText

                Layout.alignment: Qt.AlignVCenter

                font.pointSize: 8
                color: JamiTheme.textColor
            }
        }
    }
}
