import QtQuick 2.1
import QtQml.Models 2.1

import QtQuick.Controls 1.0
import QtQuick.Controls.Private 1.0
import QtQuick.Controls.Styles 1.0

import Melosic.Playlist 1.0

import "secstomins.js" as SecsToMins

DelegateModel {
    id: root
    property CategoryListView currentPlaylist
    property QtObject currentModel
    property int currentIndex

    model: playlistManagerModel

    delegate: Package {
        property Component si: StyleItem {
            elementType: "itemrow"
            horizontal: loader.horizontal
            selected: loader.selected
        }

        Loader {
            id: loader
            Package.name: 'chooser'
            property var lv: ListView.view
            property bool horizontal: lv.orientation === Qt.Horizontal
            property bool selected: loader.ListView.isCurrentItem
            property int hpadding: lv.padding
            property int vpadding: horizontal ? Math.ceil((lv.height - lbl.height) / 2) : 0
            sourceComponent: !loader.lv.tabs ? si : tabstyleloader.item ? tabstyleloader.item.tab : undefined

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
                onClicked: loader.lv.currentIndex = model.index
            }
        }

        Item {
            Package.name: 'viewer'
            id: viewerItem
            width: clv.lv.width
            height: clv.lv.height
            Connections {
                target: clv.lv
                onCurrentIndexChanged: {
                    if(viewerItem.ListView.isCurrentItem) {
                        currentPlaylist = clv
                        currentModel = playlistModel
                    }
                }
            }

            CategoryListView {
                id: clv
                property var lv: viewerItem.ListView.view
                anchors.fill: parent
                model: playlistModel
                frameVisible: true
                focus: viewerItem.ListView.isCurrentItem
                padding: 3
                removeCallback: function(from,count) { return playlistModel.removeRows(from, count) }
                moveCallback: function(from,count,to) { return playlistModel.moveRows(from, count, to) }

                category: Category {
                    id: __category

                    CategoryRole { role: "artist" }
                    CategoryRole { role: "album" }

                    delegate: Column {
                        spacing: clv.lv.padding

                        y: spacing
                        x: spacing
                        height: childrenRect.height + spacing*(children.length)
                        width: clv.lv.width
                        Label {
                            text: model.artist + " - " + model.album
                            elide: Text.ElideRight
                            color: textColor
                            width: parent.width
                        }
                        Label {
                            text: model.genre + " | " + model.year + " | " + itemCount + " tracks"
                            elide: Text.ElideRight
                            color: textColor
                            width: parent.width
                        }
                    }
                }

                delegate: Row {
                    id: track
                    spacing: clv.lv.padding
                    x: spacing
                    Row {
                        spacing: parent.spacing
                        width: itemWidth - durationLbl.width - (spacing*3)
                        Label {
                            id: trackno
                            elide: Text.ElideRight
                            color: textColor
                            text: model.tracknumber
                            width: 15
                        }
                        Label {
                            x: spacing
                            elide: Text.ElideRight
                            color: textColor
                            text: model.title
                            width: parent.width - trackno.width - spacing
                        }
                    }
                    Label {
                        id: durationLbl
                        width: contentWidth

                        color: textColor
                        text: SecsToMins.secsToMins(model.duration)
                    }
                }
            }
        }
    }
}
