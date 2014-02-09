import QtQuick 2.2

import QtQuick.Dialogs 1.1
import QtQuick.Controls 1.1
import QtQuick.Layouts 1.1
import QtQuick.Window 2.1

import Melosic.Playlist 1.0
import Melosic.Browser 1.0

ColumnLayout {
    id: root
//    orientation: Qt.Vertical

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
//            var emodel = new FilterPane
            var pane = paneLoader.createObject(root,
                {
                    model: __panes[i].model,
                    selectionModel: __panes[i].selectionModel
                })
        }
    }

    Component {
        id: paneLoader
        SelectorView {
            id: tableView
//                width: 100
            Layout.minimumWidth: 100
            Layout.alignment: Qt.AlignLeft
            Layout.fillWidth: true
//            itemDelegate: Label {
//                text: modelData.document
////                color: ListView.isCurrentItem ? "blue" : "black"
//            }

//            selectionMode: SelectionMode.ExtendedSelection
//            property var columnTitle
//            TableViewColumn {
//                role: "display"
//                title: tableView.columnTitle
//            }
//            itemDelegate: Label {
//                text: styleData.value
//            }
        }
    }
}
