import QtQuick 2.2

import QtQuick.Controls 1.1
import QtQuick.Controls.Private 1.0
import QtQuick.Controls.Styles 1.1

import Melosic.Playlist 1.0

import "secstomins.js" as SecsToMins

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

    ScrollView {
        anchors.fill: parent
    ListView {
        id: lv

        focus: true
        interactive: false
        anchors.fill: parent

        property int padding: 3
        property bool tabs: false
        property int tabPosition: Qt.TopEdge
        currentIndex: manager.currentIndex

        model: playlistManagerModel
        delegate: Loader {
            id: loader
            property var lv: ListView.view
            property bool horizontal: lv.orientation === Qt.Horizontal
            property bool selected: loader.ListView.isCurrentItem
            sourceComponent: !loader.lv.tabs ? si : tabstyleloader.item ? tabstyleloader.item.tab : undefined

            property int index: model.index

            property string text: model["display"]

            property QtObject styleData: QtObject {
                readonly property alias index: loader.index
                readonly property alias selected: loader.selected
                readonly property alias title: loader.text
                readonly property bool nextSelected: loader.lv.currentIndex === index + 1
                readonly property bool previousSelected: loader.lv.currentIndex === index - 1
                readonly property alias hovered: mouseItem.containsMouse
                readonly property bool activeFocus: loader.lv.activeFocus
//                readonly property real availableWidth: loader.availableWidth
            }

            property var __control: loader.lv
            z: selected ? 1 : -index

            Component {
                id: si
                StyleItem {
                    elementType: "item"
                    horizontal: loader.horizontal
                    selected: loader.selected
                    text: loader.text
                }
            }

            Loader {
                id: tabloader
                Component {
                    id: t_
                    TabView {
                        visible: false
                        height: 0
                        width: 0
                    }
                }
                sourceComponent: loader.lv.tabs ? t_ : undefined
            }
            Loader {
                id: tabstyleloader
                sourceComponent: tabloader.item ? tabloader.item.style : undefined
                property var __control: loader.lv
            }

            MouseArea {
                id: mouseItem
                z: loader.z+2
                anchors.fill: parent
                acceptedButtons: Qt.LeftButton
                onClicked: manager.currentIndex = model.index
            }
        }
    }
    }
}
