import QtQuick 2.2

import QtQuick.Dialogs 1.1
import QtQuick.Controls 1.1
import QtQuick.Layouts 1.1
import QtQuick.Window 2.1
import QtQuick.Controls.Private 1.0
import QtQuick.Controls.Styles 1.1

import Melosic.Playlist 1.0
import Melosic.Browser 1.0

SplitView {
    id: root
    orientation: Qt.Vertical

    default property alias __panes: panes.data

    Item {
        id: panes
        visible: false
    }

    Component.onCompleted: {
        for (var i=0; i<__panes.length; ++i) {
            if(!__panes[i].hasOwnProperty("header"))
                continue
            if(!__panes[i].hasOwnProperty("model"))
                continue
            var props = {
                model: __panes[i].model,
                selectionModel: __panes[i].selectionModel
            }
            if(__panes[i].delegate !== null)
                props.itemDelegate = __panes[i].delegate
            var pane = paneLoader.createObject(root, props)
            root.addItem(pane)
        }
    }

    Component {
        id: paneLoader
        SelectorView {
            id: tableView
            Layout.minimumHeight: 100
            Layout.alignment: Qt.AlignLeft
            Layout.fillWidth: true
            itemDelegate: Label {
                id: lbl
                x: 1
                width: tableView.width-2
                text: documentstring
                color: styleData.textColor
            }
            styleDelegate: StyleItem {
                elementType: "itemrow"
                active: tableView.activeFocus
                selected: isSelected
            }
            focusDelegate: StyleItem {
                elementType: "focusrect"
                visible: isCurrent
            }
        }
    }
}
