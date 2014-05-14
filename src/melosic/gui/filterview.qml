import QtQuick 2.2

import QtQuick.Dialogs 1.1
import QtQuick.Controls 1.1
import QtQuick.Layouts 1.1
import QtQuick.Window 2.1
import QtQuick.Controls.Private 1.0
import QtQuick.Controls.Styles 1.1

import Melosic.Playlist 1.0
import Melosic.Browser 1.0

DynamicSplitView {
    id: root
    orientation: Qt.Vertical
    signal activated(var model)

    default property alias __panes: panes.data

    Item {
        id: panes
        visible: false
    }

    property var filterResultPane

    Component {
        id: dragComponent
        Item {
            objectName: "filterDragItem"

            Drag.hotSpot.x: 0
            Drag.hotSpot.y: 0
            Drag.dragType: Drag.Automatic

            function getSelectionModel() {
                return filterResultPane.selectionModel
            }

            Drag.onDragStarted: {
                console.debug("Filter drag started")
            }
            Drag.onDragFinished: {
                console.debug("Filter drag finished")
            }
        }
    }

    Component.onCompleted: {
        console.assert(filterResultPane, "The default result filter should be set.")
        for (var i=0; i<__panes.length; ++i) {
            if(!__panes[i].hasOwnProperty("selectionModel"))
                continue
            if(!__panes[i].hasOwnProperty("model"))
                continue
            var props = {
                model: __panes[i].model,
                selectionModel: __panes[i].selectionModel,
                draggable: dragComponent
            }

            if(__panes[i].delegate !== null)
                props.itemDelegate = __panes[i].delegate
            if(__panes[i].header !== undefined)
                props.headerText = __panes[i].header

            var pane = paneLoader.createObject(root, props)

            if(__panes[i] === filterResultPane) {
                pane.activated.connect(function(items) {
                    root.activated(items)
                })
            }
            else
                pane.activated.connect(function(items) {
                    root.activated(filterResultPane.model)
                })

            root.addItem(pane)
        }
    }

    Component {
        id: paneLoader
        SelectorView {
            id: selectorView
            Layout.minimumHeight: 100
            Layout.alignment: Qt.AlignLeft
            Layout.fillWidth: true

            property var headerText
            header: Loader {
                active: headerText
                sourceComponent: Label {
                    text: headerText
                }
            }

            selectionMimeType: "melo/filter.selection"

            styleDelegate: StyleItem {
                elementType: "itemrow"
                active: selectorView.activeFocus
                selected: isSelected
            }
            focusDelegate: StyleItem {
                elementType: "focusrect"
                visible: isCurrent
            }
        }
    }
}
