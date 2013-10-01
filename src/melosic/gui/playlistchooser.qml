import QtQuick 2.1

import QtQuick.Controls 1.0
import QtQuick.Controls.Private 1.0
import QtQuick.Controls.Styles 1.0

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

    ListView {
        id: lv

        focus: true
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
            property int hpadding: lv.padding
            property int vpadding: horizontal ? Math.ceil((lv.height - lbl.height) / 2) : 0
            sourceComponent: !loader.lv.tabs ? si : tabstyleloader.item ? tabstyleloader.item.tab : undefined

            Binding on height {
                when: !loader.lv.tabs
                value: loader.horizontal ? loader.lv.height : lbl.height + (loader.vpadding * 2)
            }
            Binding on width {
                when: !loader.lv.tabs
                value: loader.horizontal ? lbl.width + (loader.hpadding * 2) : loader.lv.width
            }

            property int index: model.index

            property QtObject styleData: QtObject {
                readonly property alias index: loader.index
                readonly property alias selected: loader.selected
                readonly property alias title: lbl.text
                readonly property bool nextSelected: loader.lv.currentIndex === index + 1
                readonly property bool previousSelected: loader.lv.currentIndex === index - 1
                readonly property alias hovered: mouseItem.containsMouse
                readonly property bool activeFocus: loader.lv.activeFocus
//                readonly property real availableWidth: loader.availableWidth
            }

            property var __control: loader.lv
            z: __control.z +1

            Component {
                id: si
                StyleItem {
                    elementType: "itemrow"
                    horizontal: loader.horizontal
                    selected: loader.selected
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

            Label {
                z: loader.z + 1
                id: lbl
                x: loader.hpadding
                y: loader.vpadding + (loader.lv.tabs * !loader.ListView.isCurrentItem * 2)
                visible: !loader.lv.tabs
                text: model["display"]
                color: loader.lv.tabs ? pal.text : loader.ListView.isCurrentItem ? pal.highlightedText : pal.text
            }

            MouseArea {
                id: mouseItem
                z: lbl.z+1
                anchors.fill: parent
                acceptedButtons: Qt.LeftButton
                onClicked: manager.currentIndex = model.index
            }
        }
    }
}
