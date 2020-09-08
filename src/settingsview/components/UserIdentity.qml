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

ColumnLayout {
    id: root

    property int itemWidth
    property bool isSIP

    function updateAccountInfo() {
        if (!isSIP) {
            jamiUserIdentity.updateAccountInfo()
        } else {
            sipUserIdentity.updateAccountInfo()
        }
    }

    ElidedTextLabel {
        Layout.fillWidth: true
        Layout.preferredHeight: JamiTheme.preferredFieldHeight

        eText: qsTr("Identity")
        maxWidth: root.width - 72
        fontSize: JamiTheme.headerFontSize
    }

    JamiUserIdentity {
        id: jamiUserIdentity
        visible: !root.isSIP
        itemWidth: root.itemWidth

        Layout.fillWidth: true
        Layout.leftMargin: JamiTheme.preferredMarginSize
    }

    SIPUserIdentity {
        id: sipUserIdentity
        visible: root.isSIP
        itemWidth: root.itemWidth

        Layout.fillWidth: true
        Layout.leftMargin: JamiTheme.preferredMarginSize
    }
}
