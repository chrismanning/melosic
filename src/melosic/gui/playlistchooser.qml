import QtQuick 2.1

import Melosic.Playlist 1.0

Rectangle {
    id: root
    width: 100
    height: 50
    color: pal.light

    property alias currentIndex: lv.currentIndex
    property alias currentItem: lv.currentItem
    property alias orientation: lv.orientation
    property string textRole: "display"
    property string elementType: "itemrow"
    property alias spacing: lv.spacing
    property int padding: 3
    property PlaylistManager manager
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
    }
}
