import QtQuick 2.2
import QtQuick.Controls 1.1
import QtQuick.Controls.Private 1.0
import QtQuick.Window 2.1

import Melosic.Browser 1.0

ScrollView {
    id: root

    property alias model: listView.model
    frameVisible: true

    property var selectionModel

    property alias delegate: listView.delegate
    property Component itemDelegate
    property Component styleDelegate
    property Component focusDelegate

    ListView {
        id: listView
        focus: true
        anchors.fill: parent
        currentIndex: selectionModel ? selectionModel.currentRow : -1
        interactive: Settings.hasTouchScreen

        SystemPalette {
            id: palette
            colorGroup: root.enabled ? SystemPalette.Active : SystemPalette.Disabled
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
                    mouseSelect(newIndex, mouse.modifiers)
                    mouseArea.clickedRow = newIndex
                }
                mouseModifiers = mouse.modifiers
            }

            function mouseSelect(index, modifiers) {
                if(!selectionModel)
                    return
                selectionModel.currentRow = index
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
            height: styleloader.height
            width: root.width

            readonly property var itemModelData: typeof modelData == "undefined" ? null : modelData
            readonly property var itemModel: model
            readonly property int rowIndex: model.index
            readonly property color itemTextColor: isSelected ? palette.highlightedText : palette.text

            property bool isCurrent: ListView.isCurrentItem
            property bool isSelected
            Connections {
                target: selectionModel
                onSelectionChanged: {
                    item.isSelected = selectionModel.isSelected(index)
                }
            }

            Loader {
                id: itemloader
                z: -1
                width: item.width
                property alias isSelected: item.isSelected
                property alias isCurrent: item.isCurrent
                property string documentstring: itemModel.documentstring

                // these properties are exposed to the item delegate
                readonly property var model: listView.model
                readonly property var modelData: itemModelData
                readonly property var itemModel: item.itemModel

                property QtObject styleData: QtObject {
                    readonly property int row: item.rowIndex
                    readonly property bool selected: item.isSelected
                    readonly property color textColor: item.itemTextColor
                    readonly property var itemModel: itemloader.itemModel
                }
                sourceComponent: itemDelegate
            }

            Loader {
                id: styleloader
                z: -3
                width: item.width
                property alias isSelected: item.isSelected
                property alias isCurrent: item.isCurrent
                sourceComponent: styleDelegate
            }

            Loader {
                id: focusloader
                z: -2
                anchors.fill: parent
                property alias isSelected: item.isSelected
                property alias isCurrent: item.isCurrent
                sourceComponent: focusDelegate
            }
        }
    }
}
