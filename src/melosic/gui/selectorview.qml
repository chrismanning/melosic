import QtQuick 2.2
import QtQuick.Controls 1.2
import QtQuick.Controls.Private 1.0
import QtQuick.Window 2.1

import Melosic.Browser 1.0

ScrollView {
    id: root

    property alias model: listView.model
    frameVisible: true

    property string selectionMimeType

    property var selectionModel

    property Component header

    property var contextMenu

    highlightOnFocus: true

    property alias delegate: listView.delegate
    property Component itemDelegate
    property Component styleDelegate
    property Component focusDelegate

    property Component draggable

    property bool activateItemOnSingleClick: false

    signal activated(var selection)

    Keys.onPressed: {
        if(event.key === Qt.Key_Up) {
            up(1, event.modifiers)
            event.accepted = true
        }
        else if(event.key === Qt.Key_Down) {
            down(1, event.modifiers)
            event.accepted = true
        }
        else if(event.key === Qt.Key_PageUp) {
            if(!listView.currentItem)
                return
            var idx = listView.indexAt(root.width/2, listView.currentItem.y + listView.currentItem.height - listView.height)
            if(idx === -1)
                idx = 0
            up(selectionModel.currentRow - idx, event.modifiers)
            event.accepted = true
        }
        else if(event.key === Qt.Key_PageDown) {
            if(!listView.currentItem)
                return
            var idx = listView.indexAt(root.width/2, listView.currentItem.y + listView.height)
            if(idx === -1)
                idx = listView.count-1
            down(idx - selectionModel.currentRow, event.modifiers)
            event.accepted = true
        }
        else if(event.key === Qt.Key_Home) {
            up(listView.count, event.modifiers)
            event.accepted = true
        }
        else if(event.key === Qt.Key_End) {
            down(listView.count, event.modifiers)
            event.accepted = true
        }
        else if(event.key === Qt.Key_Space) {
            if(event.modifiers & Qt.ControlModifier)
                selectionModel.select(selectionModel.currentRow, SelectionModel.Toggle)
            else
                selectionModel.select(selectionModel.currentRow, SelectionModel.Select)
            event.accepted = true
        }
        else if(event.key === Qt.Key_Enter || event.key === Qt.Key_Return) {
            activated(selectionModel)
            event.accepted = true
        }
        else if(event.matches(StandardKey.SelectAll)) {
            if(!selectionModel || !listView.count)
                return
            selectionModel.select(0, listView.count-1, SelectionModel.Select)
            event.accepted = true
        }
        else if(event.matches(StandardKey.Deselect)) {
            if(!selectionModel || !listView.count)
                return
            selectionModel.select(0, listView.count-1, SelectionModel.Clear)
            event.accepted = true
        }
        else if(event.key === Qt.Key_Menu) {
            if(contextMenu)
                contextMenu.popup()
            event.accepted = true
        }
    }

    function up(n, modifiers) {
        if(!selectionModel || selectionModel.currentRow <= 0)
            return

        if(selectionModel.currentRow-n < 0)
            n = selectionModel.currentRow

        if(modifiers & Qt.ShiftModifier)
            selectionModel.select(selectionModel.currentRow, selectionModel.currentRow-n, SelectionModel.Select)
        else if(modifiers & Qt.ControlModifier) {}
        else
            selectionModel.select(selectionModel.currentRow-n, SelectionModel.ClearAndSelect)

        selectionModel.currentRow -= n
    }

    function down(n, modifiers) {
        if(!selectionModel || selectionModel.currentRow >= listView.count)
            return

        if(selectionModel.currentRow + n >= listView.count)
            n = listView.count - selectionModel.currentRow - 1

        if(modifiers & Qt.ShiftModifier)
            selectionModel.select(selectionModel.currentRow, selectionModel.currentRow+n, SelectionModel.Select)
        else if(modifiers & Qt.ControlModifier) {}
        else
            selectionModel.select(selectionModel.currentRow+n, SelectionModel.ClearAndSelect)
        selectionModel.currentRow += n
    }

    Loader {
        id: headerLoader
        width: root.width
        sourceComponent: root.header
    }

    __viewTopMargin: headerLoader.height

    ListView {
        id: listView
        z: headerLoader.z -1
        focus: true
        anchors.top: parent.bottom
        anchors.topMargin: headerLoader.height
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right

        currentIndex: selectionModel ? selectionModel.currentRow : -1
        interactive: Settings.hasTouchScreen
        boundsBehavior: Flickable.StopAtBounds

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
            anchors.top: parent.top
            anchors.topMargin: listView.headerItem ? listView.headerItem.height : 0
            anchors.bottom: parent.bottom
            anchors.bottomMargin: listView.footerItem ? listView.footerItem.height : 0
            anchors.left: parent.left
            anchors.right: parent.right

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
                var clickIndex = listView.indexAt(0, mouse.y + listView.contentY + anchors.topMargin)
                if(clickIndex >= 0 && root.activateItemOnSingleClick)
                    root.activated(selectionModel)
            }

            property int pressModifiers: 0
            onPressed: {
                root.forceActiveFocus()

                if(!selectionModel)
                    return

                pressModifiers = mouse.modifiers

                var newIndex = listView.indexAt(0, mouse.y + listView.contentY + anchors.topMargin)
                if(newIndex <= -1)
                    selectionModel.clearSelection()
                else if(mouse.modifiers & Qt.ControlModifier) {
                    if(mouse.modifiers & Qt.ShiftModifier)
                        selectionModel.select(selectionModel.currentRow, newIndex, SelectionModel.Select)
                    else
                        selectionModel.select(newIndex, SelectionModel.Toggle)
                }
                else if(!selectionModel.isSelected(newIndex)) {
                    if(mouse.modifiers & Qt.ShiftModifier)
                        selectionModel.select(selectionModel.currentRow, newIndex, SelectionModel.ClearAndSelect)
                    else
                        selectionModel.select(newIndex, SelectionModel.ClearAndSelect)
                }
                else {
                    selectionModel.select(newIndex, SelectionModel.Select)
                }
                selectionModel.currentRow = newIndex

                if(mouse.button == Qt.RightButton && contextMenu)
                    contextMenu.popup()
            }

            onReleased: {
                if(!selectionModel)
                    return

                var newIndex = listView.indexAt(0, mouse.y + listView.contentY + anchors.topMargin)
                if(selectionModel.isSelected(newIndex)
                        && !(pressModifiers & Qt.ControlModifier)
                        && !(pressModifiers & Qt.ShiftModifier)
                        && mouse.button != Qt.RightButton)
                    selectionModel.select(newIndex, SelectionModel.ClearAndSelect)

                pressModifiers = 0
            }

            onDoubleClicked: {
                if(mouse.button == Qt.RightButton)
                    return
                var clickIndex = listView.indexAt(0, mouse.y + listView.contentY + anchors.topMargin)
                if(clickIndex > -1 && !root.activateItemOnSingleClick)
                    root.activated(selectionModel)
            }
        }

        delegate: Item {
            id: item
            height: styleloader.height
            width: root.viewport.width

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
