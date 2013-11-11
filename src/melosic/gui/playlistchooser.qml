import QtQuick 2.2

import QtQuick.Controls 1.1
import QtQuick.Controls.Private 1.0
import QtQuick.Controls.Styles 1.1
import QtQuick.Window 2.1
import QtQuick.Layouts 1.1

import Melosic.Playlist 1.0

import "secstomins.js" as SecsToMins

Rectangle {
    id: root
    color: "transparent"

    property alias background: root.color
    property alias currentIndex: lv.currentIndex
    property alias currentItem: lv.currentItem
    property alias orientation: lv.orientation
    property string textRole: "display"
    property string elementType: "itemrow"
    property alias spacing: lv.spacing
    property alias padding: lv.padding
    property PlaylistManager manager
    property alias tabs: lv.tabs

    Window {
        id: renameDialog
        title: "Rename Playlist"
        SystemPalette {id: syspal}
        color: syspal.window
        maximumWidth: 200
        maximumHeight: 70
        minimumWidth: maximumWidth
        minimumHeight: maximumHeight

        property QtObject playlistModel: null

        modality: Qt.WindowModal
        flags: Qt.Dialog | Qt.WindowCloseButtonHint

        ColumnLayout {
            width: renameDialog.width
            height: renameDialog.height
            TextField {
                id: nameField
                Layout.alignment: Qt.AlignHCenter
            }
            RowLayout {
                id: btnRow
                Layout.alignment: Qt.AlignHCenter
                Button {
                    id: okBtn
                    iconName: "dialog-ok"
                    text: "OK"
                    onClicked: {
                        if(renameDialog.playlistModel === null || nameField.text.length <= 0)
                            return
                        renameDialog.playlistModel.name = nameField.text
                        nameField.text = ""
                        renameDialog.close()
                    }
                }
                Button {
                    id: cancelBtn
                    iconName: "dialog-cancel"
                    text: "Cancel"
                    onClicked: {
                        nameField.text = ""
                        renameDialog.close()
                    }
                }
            }
        }
    }

    Menu {
        id: contextMenu
        MenuItem {
            text: "Rename"
            iconName: "edit-rename"
            onTriggered: {
                var item = lv.itemAt(mouseItem.mouseX, mouseItem.mouseY)
                nameField.text = item.text
                renameDialog.playlistModel = item.playlistModel
                renameDialog.visible = true
            }
        }
        MenuItem {
            text: "Insert new"
            iconName: "list-add"
            onTriggered: playlistManagerModel.insertRows(lv.indexAt(mouseItem.mouseX, mouseItem.mouseY),1)
        }
        MenuItem {
            text: "Remove"
            iconName: "list-remove"
            onTriggered: playlistManagerModel.removeRows(lv.indexAt(mouseItem.mouseX, mouseItem.mouseY),1)
        }
    }

    ScrollView {
        anchors.fill: parent
    ListView {
        id: lv

        focus: true
        interactive: false
        anchors.fill: parent

        property int padding: 3
        property bool tabs: false
        property int tabPosition: Qt.TopEdge
        currentIndex: manager.currentIndex

        MouseArea {
            id: mouseItem
            z: lv.z+2
            anchors.fill: parent
            acceptedButtons: Qt.LeftButton | Qt.RightButton
            onClicked: {
                if(lv.indexAt(mouse.x, mouse.y) === -1)
                    return
                if(mouse.button == Qt.LeftButton)
                    manager.currentIndex = lv.indexAt(mouse.x, mouse.y)
                if(mouse.button == Qt.RightButton)
                    contextMenu.popup(mouse.x, mouse.y)
            }
            onDoubleClicked: {
                if(lv.indexAt(mouse.x, mouse.y) === -1)
                    playlistManagerModel.insertRows(lv.count, 1)
            }
        }

        model: playlistManagerModel
        delegate: Loader {
            id: loader
            property var lv: ListView.view
            property bool horizontal: lv.orientation === Qt.Horizontal
            property bool selected: loader.ListView.isCurrentItem
            sourceComponent: !loader.lv.tabs ? si : tabstyleloader.item ? tabstyleloader.item.tab : undefined

            property int index: model.index
            property QtObject playlistModel: model.playlistModel

            property string text: playlistModel.name

            MouseArea {
                id: internalMouseItem
                hoverEnabled: true
            }

            property QtObject styleData: QtObject {
                readonly property alias index: loader.index
                readonly property alias selected: loader.selected
                readonly property alias title: loader.text
                readonly property bool nextSelected: loader.lv.currentIndex === index + 1
                readonly property bool previousSelected: loader.lv.currentIndex === index - 1
                readonly property alias hovered: internalMouseItem.containsMouse
                readonly property bool activeFocus: loader.lv.activeFocus
//                readonly property real availableWidth: loader.availableWidth
            }

            property var __control: loader.lv
            z: selected ? 1 : -index

            Component {
                id: si
                StyleItem {
                    elementType: "item"
                    horizontal: loader.horizontal
                    selected: loader.selected
                    text: loader.text
                }
            }

            Loader {
                id: tabloader
                Component {
                    id: t_
                    TabView {
                        visible: false
                        height: 0
                        width: 0
                    }
                }
                sourceComponent: loader.lv.tabs ? t_ : undefined
            }
            Loader {
                id: tabstyleloader
                sourceComponent: tabloader.item ? tabloader.item.style : undefined
                property var __control: loader.lv
            }
        }
    }
    }
}
