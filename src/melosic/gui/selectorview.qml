import QtQuick 2.2
import QtQuick.Controls 1.1
import QtQuick.Controls.Private 1.0
import QtQuick.Window 2.1

import Melosic.Browser 1.0

ScrollView {
    id: root

    property alias model: listView.model
    frameVisible: true

    property string selectionMimeType

    property var selectionModel

    property alias delegate: listView.delegate
    property Component itemDelegate
    property Component styleDelegate
    property Component focusDelegate

    property Component draggable

    property bool activateItemOnSingleClick: false

    signal activated(var selection)

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
            acceptedButtons: Qt.LeftButton | Qt.RightButton
            propagateComposedEvents: true
            preventStealing: !Settings.hasTouchScreen
            drag.threshold: 5

            Component.onCompleted: {
                var mime = {}
                mime[root.selectionMimeType] = root.selectionModel
                drag.target = draggable.createObject(mouseArea, {
                                                         "Drag.active":
                                                            Qt.binding(function(){ return mouseArea.drag.active }),
                                                         "Drag.mimeData": mime
                                                     })
            }

            onClicked: {
                var clickIndex = listView.indexAt(0, mouseY + listView.contentY)
                if(clickIndex > -1) {
                    if(root.activateItemOnSingleClick)
                        root.activated(selectionModel)
                }
            }

            property var pressModifiers
            onPressed: {
                pressModifiers = mouse.modifiers
                var newIndex = listView.indexAt(0, mouseY + listView.contentY)
                root.forceActiveFocus()

                if(!selectionModel)
                    return

                if(newIndex <= -1)
                    selectionModel.clearSelection()
                else if(!selectionModel.isSelected(newIndex)) {
                    mouseSelect(newIndex, mouse.modifiers)
                    selectionModel.currentRow = newIndex
                }
            }

            onPositionChanged: {
                if(!pressModifiers)
                    pressModifiers = 1
            }

            onReleased: {
                var newIndex = listView.indexAt(0, mouseY + listView.contentY)
                if(!(pressModifiers & Qt.ShiftModifier) && !(pressModifiers & Qt.ControlModifier) &&
                        selectionModel.isSelected(newIndex))
                {
                    mouseSelect(newIndex, pressModifiers)
                    selectionModel.currentRow = newIndex
                }
            }

            function mouseSelect(index, modifiers) {
                if(!selectionModel)
                    return
                if(modifiers & Qt.ShiftModifier)
                    selectionModel.select(selectionModel.currentRow, index, SelectionModel.ClearAndSelect)
                else if(modifiers & Qt.ControlModifier)
                    selectionModel.select(index, SelectionModel.Toggle)
                else
                    selectionModel.select(index, SelectionModel.ClearAndSelect)
            }

            onDoubleClicked: {
                var clickIndex = listView.indexAt(0, mouseY + listView.contentY)
                if(clickIndex > -1) {
                    if(!root.activateItemOnSingleClick)
                        root.activated(selectionModel)
                }
            }
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
                onSelectionChanged: item.isSelected = selectionModel.isSelected(index)
            }

            Loader {
                id: itemloader
                z: -1
                width: item.width
                property alias isSelected: item.isSelected
                property alias isCurrent: item.isCurrent
                property string documentstring: itemModel.documentstring
                property var document: itemModel.document

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
