/*
 * Copyright (C) 2020 by Savoir-faire Linux
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

import QtQuick 2.14
import QtQuick.Controls 2.14
import QtQuick.Layouts 1.14

import net.jami.Models 1.0
import net.jami.Adapters 1.0
import net.jami.Constants 1.0

import "../../commoncomponents"

ModalPopup {
    id: root

    property alias preferredHeight: aboutPopUpContentRectColumnLayout.implicitHeight

    contentItem: Rectangle {
        id: contentRect

        implicitWidth: 400
        color: JamiTheme.backgroundColor

        ScrollView {
            id: aboutPopUpScrollView

            anchors.fill: parent

            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
            ScrollBar.vertical.policy: ScrollBar.AsNeeded

            clip: true

            ColumnLayout {
                width: Math.max(root.width, implicitWidth)
                height: Math.max(aboutPopUpScrollView.height, implicitHeight)

                spacing: 0

                ColumnLayout {
                    id: aboutPopUpContentRectColumnLayout

                    Layout.alignment: Qt.AlignCenter

                    ResponsiveImage {
                        id: aboutPopUPJamiLogoImage

                        Layout.alignment: Qt.AlignCenter
                        Layout.topMargin: JamiTheme.preferredMarginSize
                        Layout.preferredWidth: 250
                        Layout.preferredHeight: 88

                        smooth: true
                        antialiasing: true

                        source: JamiTheme.darkTheme? "qrc:/images/logo-jami-standard-coul-white.svg" : "qrc:/images/logo-jami-standard-coul.svg"
                    }

                    Label {
                        id: jamiVersionText

                        Layout.alignment: Qt.AlignCenter
                        Layout.preferredWidth: contentRect.width
                        Layout.preferredHeight: textMetricsjamiVersionText.boundingRect.height

                        font.pointSize: JamiTheme.textFontSize

                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter

                        text: textMetricsjamiVersionText.text
                        color: JamiTheme.textColor

                        TextMetrics {
                            id: textMetricsjamiVersionText
                            font: jamiVersionText.font
                            text: JamiStrings.version + ": " + UtilsAdapter.getVersionStr()
                        }
                    }

                    Label {
                        id: jamiSlogansText

                        Layout.alignment: Qt.AlignCenter
                        Layout.preferredWidth: contentRect.width
                        Layout.preferredHeight: textMetricsjamiSlogansText.boundingRect.height
                        Layout.topMargin: 5

                        wrapMode: Text.WordWrap
                        font.pointSize: JamiTheme.textFontSize

                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter

                        text: textMetricsjamiSlogansText.text
                        color: JamiTheme.textColor

                        TextMetrics {
                            id: textMetricsjamiSlogansText
                            font: jamiSlogansText.font
                            text: JamiStrings.slogan
                        }
                    }

                    Label {
                        id: jamiDeclarationText

                        Layout.alignment: Qt.AlignCenter
                        Layout.preferredWidth: contentRect.width
                        Layout.preferredHeight: 40
                        Layout.topMargin: 5

                        wrapMode: Text.WordWrap
                        font.pointSize: JamiTheme.textFontSize
                        color: JamiTheme.textColor

                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter


                        // TextMetrics does not work for multi-line.
                        text: JamiStrings.declaration
                    }

                    Label {
                        id: jamiDeclarationHyperText

                        Layout.alignment: Qt.AlignCenter

                        // Strangely, hoveredLink works badly when width grows too large
                        Layout.preferredWidth: 50
                        Layout.preferredHeight: textMetricsjamiDeclarationHyperText.boundingRect.height
                        Layout.topMargin: 5
                        Layout.bottomMargin: 5
                        color: JamiTheme.textColor

                        font.pointSize: JamiTheme.textFontSize
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter

                        text: textMetricsjamiDeclarationHyperText.text
                        onLinkActivated: Qt.openUrlExternally(link)

                        TextMetrics {
                            id: textMetricsjamiDeclarationHyperText
                            font: jamiDeclarationHyperText.font
                            text: '<html><style type="text/css"></style><a href="https://jami.net">jami.net</a></html>'
                        }

                        MouseArea {
                            anchors.fill: parent

                            // We don't want to eat clicks on the Text.
                            acceptedButtons: Qt.NoButton
                            cursorShape: parent.hoveredLink ? Qt.PointingHandCursor : Qt.ArrowCursor
                        }
                    }

                    Label {
                        id: jamiDeclarationYearText

                        Layout.alignment: Qt.AlignCenter
                        Layout.preferredWidth: contentRect.width
                        Layout.preferredHeight: textMetricsjamiDeclarationYearText.boundingRect.height
                        Layout.bottomMargin: 5

                        font.pointSize: JamiTheme.textFontSize
                        color: JamiTheme.textColor

                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter

                        text: textMetricsjamiDeclarationYearText.text

                        TextMetrics {
                            id: textMetricsjamiDeclarationYearText
                            font: jamiDeclarationYearText.font
                            text: JamiStrings.companyDeclarationYear
                        }
                    }

                    Label {
                        id: jamiNoneWarrantyHyperText

                        Layout.alignment: Qt.AlignCenter
                        Layout.preferredWidth: 300
                        Layout.preferredHeight: textMetricsjamiNoneWarrantyHyperText.boundingRect.height * 2
                        Layout.bottomMargin: 10

                        wrapMode: Text.WordWrap
                        font.pointSize: JamiTheme.tinyFontSize

                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        color: JamiTheme.textColor

                        text: textMetricsjamiNoneWarrantyHyperText.text
                        onLinkActivated: Qt.openUrlExternally(link)

                        TextMetrics {
                            id: textMetricsjamiNoneWarrantyHyperText
                            font: jamiDeclarationHyperText.font
                            text: '<html><style type="text/css"></style>This program comes with absolutely no warranty.<br\>See the <a href="http://www.gnu.org/licenses/gpl-3.0.html">GNU General Public License, version 3 or later</a> for details.</html>'
                        }

                        MouseArea {
                            anchors.fill: parent
                            acceptedButtons: Qt.NoButton
                            cursorShape: parent.hoveredLink ? Qt.PointingHandCursor : Qt.ArrowCursor
                        }
                    }

                    ProjectCreditsScrollView {
                        id: projectCreditsScrollView

                        Layout.alignment: Qt.AlignCenter
                        Layout.preferredWidth: contentRect.width - JamiTheme.preferredMarginSize * 2
                        Layout.preferredHeight: 128
                        Layout.margins: JamiTheme.preferredMarginSize
                    }

                    MaterialButton {
                        id: btnClose

                        Layout.alignment: Qt.AlignHCenter
                        Layout.preferredWidth: JamiTheme.preferredFieldWidth / 2
                        Layout.preferredHeight: JamiTheme.preferredFieldHeight
                        Layout.bottomMargin: JamiTheme.preferredMarginSize

                        text: qsTr("Close")
                        color: enabled? JamiTheme.buttonTintedBlack : JamiTheme.buttonTintedGrey
                        hoveredColor: JamiTheme.buttonTintedBlackHovered
                        pressedColor: JamiTheme.buttonTintedBlackPressed
                        outlined: true

                        onClicked: {
                            close()
                        }
                    }
                }
            }
        }
    }
}
