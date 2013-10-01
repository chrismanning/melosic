import QtQuick 2.1

import Melosic.Playlist 1.0

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

    Binding {
        target: manager
        property: "currentIndex"
        value: lv.currentIndex
    }

    ListView {
        id: lv
        model: manager.parts.chooser
        focus: true
        anchors.fill: parent

        property int padding: 3
        property bool tabs: false
        property int tabPosition: Qt.TopEdge
    }
}
