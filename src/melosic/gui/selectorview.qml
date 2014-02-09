import QtQuick 2.2
import QtQuick.Controls 1.1
import QtQuick.Controls.Private 1.0
import QtQuick.Controls.Styles 1.1
import QtQuick.Window 2.1

import Melosic.Browser 1.0

ScrollView {
    id: root

    property alias model: listView.model
    frameVisible: true

    property var selectionModel

    ListView {
        id: listView
        focus: true
        anchors.fill: parent
        currentIndex: selectionModel ? selectionModel.currentRow : -1
        interactive: Settings.hasTouchScreen

        SystemPalette {
            id: palette
            colorGroup: enabled ? SystemPalette.Active : SystemPalette.Disabled
        }

        Rectangle {
            id: colorRect
            parent: viewport
            anchors.fill: parent
            color: palette.base
            z: -2
        }

        MouseArea {
            id: mouseArea
            anchors.fill: parent
            z: -1
            propagateComposedEvents: true
            preventStealing: !Settings.hasTouchScreen
            property bool autoincrement: false
            property bool autodecrement: false
            property int mouseModifiers: 0
            property int previousRow: 0
            property int clickedRow: -1
            property int dragRow: -1
            property int firstKeyRow: -1

//            onClicked: {
//                var clickIndex = listView.indexAt(0, mouseY + listView.contentY)
//                if (clickIndex > -1) {
//                    if (root.__activateItemOnSingleClick)
//                        root.activated(clickIndex)
//                    root.clicked(clickIndex)
//                }
//            }

            onPressed: {
                var newIndex = listView.indexAt(0, mouseY + listView.contentY)
                listView.forceActiveFocus()
                if(newIndex <= -1 && selectionModel)
                    selectionModel.clear()
                else if(!Settings.hasTouchScreen) {
                    listView.currentIndex = newIndex
                    mouseSelect(newIndex, mouse.modifiers)
                    mouseArea.clickedRow = newIndex
                }
                mouseModifiers = mouse.modifiers
            }

            function mouseSelect(index, modifiers) {
                if(!selectionModel)
                    return
                if (modifiers & Qt.ShiftModifier)
                    selectionModel.select(previousRow, index, SelectionModel.ClearAndSelect)
                else if (modifiers & Qt.ControlModifier)
                    selectionModel.select(index, SelectionModel.Toggle)
                else
                    selectionModel.select(index, SelectionModel.ClearAndSelect)
            }

//            onDoubleClicked: {
//                var clickIndex = listView.indexAt(0, mouseY + listView.contentY)
//                if (clickIndex > -1) {
//                    if (!root.__activateItemOnSingleClick)
//                        root.activated(clickIndex)
//                    root.doubleClicked(clickIndex)
//                }
//            }
        }

        delegate: Item {
            id: item
            height: styleitem.height
            width: root.width

            Label {
                id: lbl
                x: 1
                z: -1
                width: root.width-2
                text: document
                color: styleitem.selected ? palette.highlightedText : palette.text
            }
            StyleItem {
                id: styleitem
                z: -3
                width: root.width
                elementType: "itemrow"
                active: root.activeFocus
                Connections {
                    target: selectionModel
                    onSelectionChanged: {
                        styleitem.selected = selectionModel.isSelected(index)
                    }
                }
            }
            StyleItem {
                id: focusrect
                z: -2
                anchors.margins: -1
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                elementType: "focusrect"
                visible: item.ListView.isCurrentItem
            }
        }
    }
}
