/*
 * Copyright (C) 2021-2022 Savoir-faire Linux Inc.
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import net.jami.Models 1.1
import net.jami.Adapters 1.1
import net.jami.Constants 1.1

import "../../commoncomponents"

Control {
    id: root

    enum ActionPopupMode {
        MediaDevice = 0,
        ListElement
    }

    property alias overflowOpen: overflowButton.popup.visible
    property bool subMenuOpen: false

    property real itemSpacing: 2

    signal chatClicked
    signal addToConferenceClicked
    signal transferClicked
    signal resumePauseCallClicked
    signal showInputPanelClicked
    signal shareScreenClicked
    signal shareWindowClicked
    signal stopSharingClicked
    signal shareScreenAreaClicked
    signal shareFileClicked
    signal pluginsClicked
    signal recordCallClicked
    signal fullScreenClicked

    Component {
        id: buttonDelegate

        CallButtonDelegate {
            width: root.height
            height: width
            onSubMenuVisibleChanged: subMenuOpen = subMenuVisible
        }
    }

    Connections {
        target: AvAdapter

        function onAudioDeviceListChanged(inputs, outputs) {
            audioInputDeviceListModel.reset();
            audioInputMenuAction.enabled = inputs
            audioOutputDeviceListModel.reset();
            audioOutputMenuAction.enabled = outputs
        }
    }

    property list<Action> menuActions: [
        Action {
            id: audioInputMenuAction
            text: JamiStrings.selectAudioInputDevice
            Component.onCompleted: enabled = audioInputDeviceListModel.rowCount()
            property var listModel: AudioDeviceModel {
                id: audioInputDeviceListModel
                lrcInstance: LRCInstance
                type: AudioDeviceModel.Type.Record
            }
            function accept(index) {
                AvAdapter.stopAudioMeter()
                AVModel.setInputDevice(listModel.data(
                                       listModel.index(index, 0),
                                       AudioDeviceModel.RawDeviceName))
                AvAdapter.startAudioMeter()
            }
        },
        Action {
            id: audioOutputMenuAction
            text: JamiStrings.selectAudioOutputDevice
            Component.onCompleted: enabled = audioOutputDeviceListModel.rowCount()
            property var listModel: AudioDeviceModel {
                id: audioOutputDeviceListModel
                lrcInstance: LRCInstance
                type: AudioDeviceModel.Type.Playback
            }
            function accept(index) {
                AvAdapter.stopAudioMeter()
                AVModel.setOutputDevice(listModel.data(
                                        listModel.index(index, 0),
                                        AudioDeviceModel.RawDeviceName))
                AvAdapter.startAudioMeter()
            }
        },
        Action {
            id: shareMenuAction
            text: JamiStrings.selectShareMethod
            property int popupMode: CallActionBar.ActionPopupMode.ListElement
            property var listModel: ListModel {
                id: shareModel

                Component.onCompleted: {
                    shareModel.append({"Name": JamiStrings.shareScreen,
                                       "IconSource": JamiResources.laptop_black_24dp_svg})
                    if (Qt.platform.os == "linux") {
                        shareModel.append({"Name": JamiStrings.shareWindow,
                                           "IconSource" : JamiResources.window_black_24dp_svg})
                    }
                    shareModel.append({"Name": JamiStrings.shareScreenArea,
                                       "IconSource" : JamiResources.share_area_black_24dp_svg})
                    shareModel.append({"Name": JamiStrings.shareFile,
                                       "IconSource" : JamiResources.file_black_24dp_svg})
                }
            }
            function accept(index) {
                switch(shareModel.get(index).Name) {
                  case JamiStrings.shareScreen:
                      shareScreenClicked()
                      break
                  case JamiStrings.shareWindow:
                      shareWindowClicked()
                      break
                  case JamiStrings.shareScreenArea:
                      shareScreenAreaClicked()
                      break
                  case JamiStrings.shareFile:
                      shareFileClicked()
                      break
                }
            }
        },
        Action {
            id: videoInputMenuAction
            enabled: VideoDevices.listSize !== 0
            text: JamiStrings.selectVideoDevice
            property var listModel: VideoDevices.devicesSourceModel()
            function accept(index) {
                if (VideoDevices.listSize < 1)
                    return
                // TODO: change it when we can suppot showing default and
                //       current rendering device at the same time and
                //       start and stop preview logic in here should be in LRC
                VideoDevices.setDefaultDevice(index, true)
            }
        }
    ]

    property list<Action> primaryActions: [
        Action {
            id: muteAudioAction
            onTriggered: {
                var muteState = CallAdapter.getMuteState(CurrentAccount.uri)
                var modMuted = muteState === CallAdapter.MODERATOR_MUTED
                    || muteState === CallAdapter.BOTH_MUTED
                if (isAudioMuted && modMuted) {
                    muteAlertActive = true
                    muteAlertMessage = JamiStrings.participantModIsStillMuted
                }
                CallAdapter.muteThisCallToggle(!isAudioMuted)
            }
            checkable: true
            icon.source: checked ?
                             JamiResources.micro_off_black_24dp_svg :
                             JamiResources.micro_black_24dp_svg
            icon.color: checked ? "red" : "white"
            text: !checked ? JamiStrings.mute : JamiStrings.unmute
            property var menuAction: audioInputMenuAction
        },
        Action {
            id: hangupAction
            onTriggered: CallAdapter.hangUpThisCall()
            icon.source: JamiResources.ic_call_end_white_24dp_svg
            icon.color: "white"
            text: JamiStrings.hangup
            property bool hasBg: true
        },
        Action {
            id: muteVideoAction
            onTriggered: CallAdapter.videoPauseThisCallToggle(!isVideoMuted)
            checkable: true
            icon.source: checked ?
                             JamiResources.videocam_off_24dp_svg :
                             JamiResources.videocam_24dp_svg
            icon.color: checked ? "red" : "white"
            text: !checked ? JamiStrings.pauseVideo : JamiStrings.resumeVideo
            property var menuAction: videoInputMenuAction
        }
    ]

    property list<Action> secondaryActions: [
        Action {
            id: audioOutputAction
            // temp hack for missing back-end, just open device selection
            property bool openPopupWhenClicked: true
            checkable: !openPopupWhenClicked
            icon.source: JamiResources.spk_black_24dp_svg
            icon.color: "white"
            text: JamiStrings.selectAudioOutputDevice
            property var menuAction: audioOutputMenuAction
        },
        Action {
            id: addPersonAction
            onTriggered: root.addToConferenceClicked()
            icon.source: JamiResources.add_people_black_24dp_svg
            icon.color: "white"
            text: JamiStrings.addParticipants
        },
        Action {
            id: chatAction
            onTriggered: root.chatClicked()
            icon.source: JamiResources.chat_black_24dp_svg
            icon.color: "white"
            text: JamiStrings.chat
        },
        Action {
            id: fullScreenAction
            icon.source: layoutManager.isCallFullscreen ?
                            JamiResources.close_fullscreen_24dp_svg :
                            JamiResources.open_in_full_24dp_svg
            icon.color: "white"
            onTriggered: root.fullScreenClicked()
            text: layoutManager.isCallFullscreen ?
                          JamiStrings.exitFullScreen :
                          JamiStrings.fullScreen
        },
        Action {
            id: resumePauseCallAction
            onTriggered: root.resumePauseCallClicked()
            icon.source: isPaused ?
                             JamiResources.play_circle_outline_24dp_svg :
                             JamiResources.pause_circle_outline_24dp_svg
            icon.color: "white"
            text: isPaused ? JamiStrings.resumeCall : JamiStrings.pauseCall
        },
        Action {
            id: inputPanelSIPAction
            onTriggered: root.showInputPanelClicked()
            icon.source: JamiResources.ic_keypad_svg
            icon.color: "white"
            text: JamiStrings.sipInputPanel
        },
        Action {
            id: callTransferAction
            onTriggered: root.transferClicked()
            icon.source: JamiResources.phone_forwarded_24dp_svg
            icon.color: "white"
            text: JamiStrings.transferCall
        },
        Action {
            id: shareAction
            onTriggered: {
                if (sharingActive)
                    root.stopSharingClicked()
                else
                    root.shareScreenClicked()
            }
            icon.source: sharingActive ?
                             JamiResources.share_stop_black_24dp_svg :
                             JamiResources.share_screen_black_24dp_svg
            icon.color: sharingActive ?
                            "red" : "white"
            text: sharingActive ?
                      JamiStrings.stopSharing :
                      JamiStrings.shareScreen
            property real size: 34
            property var menuAction: shareMenuAction
        },
        Action {
            id: raiseHandAction
            onTriggered: CallAdapter.setHandRaised("", !CallAdapter.isHandRaised())
            checkable: true
            icon.source: JamiResources.hand_black_24dp_svg
            icon.color: checked ? JamiTheme.raiseHandColor : "white"
            text: checked ?
                      JamiStrings.lowerHand :
                      JamiStrings.raiseHand
            property real size: 34
        },
        Action {
            id: recordAction
            onTriggered: root.recordCallClicked()
            checkable: true
            icon.source: JamiResources.record_black_24dp_svg
            icon.color: checked ? "red" : "white"
            text: !checked ? JamiStrings.startRec : JamiStrings.stopRec
            property bool blinksWhenChecked: true
            property real size: 28
            onCheckedChanged: {
                CallOverlayModel.setUrgentCount(recordAction,
                                                checked ? -1 : 0)
            }
        },
        Action {
            id: pluginsAction
            onTriggered: root.pluginsClicked()
            icon.source: JamiResources.plugins_24dp_svg
            icon.color: "white"
            text: JamiStrings.viewPlugin
            enabled: PluginAdapter.callMediaHandlersListCount
        }
    ]

    property var overflowItemCount

    Connections {
        target: callOverlay

        function onIsAudioOnlyChanged() { reset() }
        function onIsSIPChanged() { reset() }
        function onIsModeratorChanged() { reset() }
        function onIsAudioMutedChanged() { reset() }
        function onIsVideoMutedChanged() { reset() }
        function onIsRecordingChanged() { reset() }
        function onLocalHandRaisedChanged() { reset() }
        function onIsConferenceCallChanged() { reset() }
    }
    Connections {
        target: CurrentAccount

        function onVideoEnabledVideoChanged() { reset() }
    }

    function reset() {
        CallOverlayModel.clearControls()

        // centered controls
        CallOverlayModel.addPrimaryControl(muteAudioAction)
        CallOverlayModel.addPrimaryControl(hangupAction)

        if (CurrentAccount.videoEnabled_Video)
            CallOverlayModel.addPrimaryControl(muteVideoAction)

        // overflow controls
        CallOverlayModel.addSecondaryControl(audioOutputAction)
        if (isConferenceCall) {
            CallOverlayModel.addSecondaryControl(raiseHandAction)
            raiseHandAction.checked = CallAdapter.isHandRaised()
        }
        if (isModerator && !isSIP)
            CallOverlayModel.addSecondaryControl(addPersonAction)
        if (isSIP) {
            CallOverlayModel.addSecondaryControl(resumePauseCallAction)
            CallOverlayModel.addSecondaryControl(inputPanelSIPAction)
            CallOverlayModel.addSecondaryControl(callTransferAction)
        }
        CallOverlayModel.addSecondaryControl(chatAction)
        if (CurrentAccount.videoEnabled_Video && !isSIP)
            CallOverlayModel.addSecondaryControl(shareAction)
        CallOverlayModel.addSecondaryControl(recordAction)
        CallOverlayModel.addSecondaryControl(pluginsAction)
        CallOverlayModel.addSecondaryControl(fullScreenAction)
        overflowItemCount = CallOverlayModel.secondaryModel().rowCount()

        muteAudioAction.checked = isAudioMuted
        recordAction.checked = CallAdapter.isRecordingThisCall()
        muteVideoAction.checked = isAudioOnly ? true : isVideoMuted
    }

    Item {
        id: centralControls
        anchors.centerIn: parent
        width: childrenRect.width
        height: root.height

        RowLayout {
            spacing: 0

            ListView {
                property bool centeredGroup: true

                orientation: ListView.Horizontal
                implicitWidth: contentWidth
                implicitHeight: contentHeight
                interactive: false

                model: CallOverlayModel.primaryModel()
                delegate: buttonDelegate
            }
        }
    }
    Item {
        id: overflowRect
        property real remainingSpace: (root.width - centralControls.width) / 2
        anchors.right: parent.right
        width: childrenRect.width
        height: root.height

        RowLayout {
            spacing: itemSpacing

            ListView {
                id: overflowItemListView

                orientation: ListView.Horizontal
                implicitWidth: contentWidth
                implicitHeight: overflowRect.height

                interactive: false
                spacing: itemSpacing

                property int overflowIndex: {
                    var maxItems = Math.floor(
                                (overflowRect.remainingSpace - 24) / root.height) - 1
                    return Math.min(overflowItemCount, maxItems)
                }
                property int nOverflowItems: overflowItemCount - overflowIndex
                onNOverflowItemsChanged: {
                    var diff = overflowItemListView.count - nOverflowItems
                    var effectiveOverflowIndex = overflowIndex
                    if (effectiveOverflowIndex === overflowItemCount - 1)
                        effectiveOverflowIndex += diff

                    CallOverlayModel.overflowIndex = effectiveOverflowIndex
                }

                model: CallOverlayModel.overflowModel()
                delegate: buttonDelegate
            }
            ComboBox {
                id: overflowButton

                visible: CallOverlayModel.overflowIndex < overflowItemCount
                width: root.height
                height: width

                model: CallOverlayModel.overflowHiddenModel()

                delegate: buttonDelegate

                indicator: null

                contentItem: ResponsiveImage {
                    color: "white"
                    source: JamiResources.more_vert_24dp_svg
                    anchors.fill: parent
                    anchors.margins: 17
                }

                background: HalfPill {
                    implicitWidth: root.height
                    implicitHeight: implicitWidth
                    radius: type === HalfPill.None ? 0 : 5
                    color: overflowButton.down ?
                               "#c4777777":
                               overflowButton.hovered ?
                                   "#c4444444" :
                                   "#c4272727"
                    type: {
                        if (overflowItemListView.count ||
                                urgentOverflowListView.count ||
                                (overflowHiddenListView.count &&
                                overflowButton.popup.visible)) {
                            return HalfPill.None
                        } else {
                            return HalfPill.Left
                        }
                    }

                    Behavior on color {
                        ColorAnimation { duration: JamiTheme.shortFadeDuration }
                    }
                }

                Item {
                    implicitHeight: children[0].contentHeight
                    width: overflowButton.width
                    anchors.bottom: parent.top
                    anchors.bottomMargin: itemSpacing
                    visible: !overflowButton.popup.visible
                    JamiListView {
                        id: urgentOverflowListView

                        spacing: itemSpacing
                        anchors.fill: parent
                        model: !overflowButton.popup.visible ?
                                   CallOverlayModel.overflowVisibleModel() :
                                   null

                        delegate: buttonDelegate

                        add: Transition {
                            NumberAnimation {
                                property: "opacity"
                                from: 0 ; to: 1.0; duration: 80
                            }
                            NumberAnimation {
                                property: "scale"
                                from: 0; to: 1.0; duration: 80
                            }
                        }
                    }
                }

                popup: Popup {
                    y: overflowButton.height + itemSpacing
                    width: overflowButton.width
                    implicitHeight: contentItem.implicitHeight
                    padding: 0

                    contentItem: JamiListView {
                        id: overflowHiddenListView
                        spacing: itemSpacing
                        implicitHeight: contentHeight
                        interactive: false
                        model: overflowButton.popup.visible ?
                                   overflowButton.delegateModel :
                                   null
                    }

                    background: Rectangle {
                        color: "transparent"
                    }
                }
            }
        }
    }
}
