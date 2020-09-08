/*
 * Copyright (C) 2020 by Savoir-faire Linux
 * Author: Aline Gondim Santos <aline.gondimsantos@savoirfairelinux.com>
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

import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Window 2.14
import QtQuick.Controls 2.15
import QtQuick.Controls.Universal 2.12
import QtGraphicalEffects 1.14
import QtQuick.Controls.Styles 1.4
import net.jami.Models 1.0
import net.jami.Adapters 1.0
import Qt.labs.platform 1.1
import "../../commoncomponents"
import "../../constant"

ColumnLayout {
    id: root

    property int itemWidth

    function updateOpenDHTSettingsInfos() {
        checkBoxEnableProxy.checked = SettingsAdapter.getAccountConfig_ProxyEnabled()
        lineEditProxy.setText(SettingsAdapter.getAccountConfig_ProxyServer())
        lineEditBootstrap.setText(SettingsAdapter.getAccountConfig_Hostname())
    }

    Text {
        Layout.fillWidth: true
        Layout.rightMargin: JamiTheme.preferredMarginSize / 2

        font.pointSize: JamiTheme.headerFontSize
        font.kerning: true

        horizontalAlignment: Text.AlignLeft
        verticalAlignment: Text.AlignVCenter

        text: qsTr("OpenDHT Configuration")
        elide: Text.ElideRight
    }

    ColumnLayout {
        Layout.fillWidth: true
        Layout.leftMargin: JamiTheme.preferredMarginSize

        ToggleSwitch {
            id: checkBoxEnableProxy

            labelText: qsTr("Enable proxy")
            fontPointSize: JamiTheme.settingsFontSize

            onSwitchToggled: {
                SettingsAdapter.setEnableProxy(checked)
                lineEditProxy.setEnabled(checked)
            }
        }

        SettingsMaterialLineEdit {
            id: lineEditProxy

            Layout.fillWidth: true
            Layout.preferredHeight: JamiTheme.preferredFieldHeight
            itemWidth: root.itemWidth
            titleField: qsTr("Proxy Address")

            onEditFinished: SettingsAdapter.setProxyAddress(textField)
        }

        SettingsMaterialLineEdit {
            id: lineEditBootstrap

            Layout.fillWidth: true
            Layout.preferredHeight: JamiTheme.preferredFieldHeight
            itemWidth: root.itemWidth
            titleField: qsTr("Bootstrap")

            onEditFinished: SettingsAdapter.setBootstrapAddress(textField)
        }
    }
}