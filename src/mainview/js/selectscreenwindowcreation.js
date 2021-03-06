/*
 * Copyright (C) 2020-2022 Savoir-faire Linux Inc.
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

// Global select screen window component, object variable for creation.
var selectScreenWindowComponent
var selectScreenWindowObject
var mainWindow

function createSelectScreenWindowObject(appWindow) {
    if (selectScreenWindowObject)
        return
    selectScreenWindowComponent = Qt.createComponent(
                "../components/SelectScreen.qml")
    mainWindow = appWindow
    if (selectScreenWindowComponent.status === Component.Ready)
        finishCreation()
    else if (selectScreenWindowComponent.status === Component.Error)
        console.log("Error loading component:",
                    selectScreenWindowComponent.errorString())
}

function finishCreation() {
    selectScreenWindowObject = selectScreenWindowComponent.createObject()
    if (selectScreenWindowObject === null) {
        // Error Handling.
        console.log("Error creating select screen object")
    }

    // Signal connection.
    selectScreenWindowObject.onClosing.connect(destroySelectScreenWindow)
}

function showSelectScreenWindow(previewId, window) {
    selectScreenWindowObject.currentPreview = previewId
    selectScreenWindowObject.window = window
    selectScreenWindowObject.show()

    var centerX = mainWindow.x + mainWindow.width / 2
    var centerY = mainWindow.y + mainWindow.height / 2

    selectScreenWindowObject.width = 0.75 * appWindow.width
    selectScreenWindowObject.height = 0.75 * appWindow.height
    selectScreenWindowObject.x = centerX - selectScreenWindowObject.width / 2
    selectScreenWindowObject.y = centerY - selectScreenWindowObject.height / 2
}

// Destroy and reset selectScreenWindowObject when window is closed.
function destroySelectScreenWindow() {
    if(!selectScreenWindowObject)
        return
    selectScreenWindowObject.destroy()
    selectScreenWindowObject = false
}
