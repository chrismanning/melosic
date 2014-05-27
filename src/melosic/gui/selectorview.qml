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

    property Component header

    property alias delegate: listView.delegate
    property Component itemDelegate
    property Component styleDelegate
    property Component focusDelegate

    property Component draggable

    property bool activateItemOnSingleClick: false

    signal activated(var selection)

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
                if(clickIndex > -1) {
                    if(root.activateItemOnSingleClick)
                        root.activated(selectionModel)
                }
            }

            property var pressModifiers
            onPressed: {
                pressModifiers = mouse.modifiers
                root.forceActiveFocus()

                if(!selectionModel)
                    return

                var newIndex = listView.indexAt(0, mouse.y + listView.contentY + anchors.topMargin)
                if(newIndex <= -1)
                    selectionModel.clearSelection()
                else {
                    if(mouse.modifiers & Qt.ShiftModifier)
                        selectionModel.select(selectionModel.currentRow, newIndex, SelectionModel.Select)
                    else if(mouse.modifiers & Qt.ControlModifier)
                        selectionModel.select(newIndex, SelectionModel.Toggle)
                    else
                        selectionModel.select(newIndex, SelectionModel.Select)
                }
            }

            onPositionChanged: {
                if(!pressModifiers)
                    pressModifiers = 1
            }

            onReleased: {
                if(!selectionModel)
                    return

                var newIndex = listView.indexAt(0, mouse.y + listView.contentY + anchors.topMargin)
                if(selectionModel.isSelected(newIndex)) {
                    if(pressModifiers & Qt.ShiftModifier) {
                        var flag = SelectionModel.ClearAndSelect
                        if(pressModifiers & Qt.ControlModifier)
                            flag = SelectionModel.Select
                        selectionModel.select(selectionModel.currentRow, newIndex, flag)
                    }
                    else if(!(pressModifiers & Qt.ControlModifier))
                        selectionModel.select(newIndex, SelectionModel.ClearAndSelect)
                }
                selectionModel.currentRow = newIndex
            }

            onDoubleClicked: {
                var clickIndex = listView.indexAt(0, mouse.y + listView.contentY + anchors.topMargin)
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
