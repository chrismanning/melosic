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
            horizontal: chooserItem.horizontal
            selected: chooserItem.selected
        }

        Item {
            Package.name: 'chooser'
            id: chooserItem
            property var lv: ListView.view
            property bool horizontal: lv.orientation === Qt.Horizontal
            property bool selected: chooserItem.ListView.isCurrentItem

            property int hpadding: lv.padding
            property int vpadding: horizontal ? Math.ceil((lv.height - lbl.height) / 2) : 0
            property bool hover: false
            height: horizontal ? lv.height : lbl.height + (chooserItem.vpadding * 2)
            width: horizontal ? lbl.width + (chooserItem.hpadding * 2) : lv.width

            Loader {
                id: loader
                anchors.fill: parent
                sourceComponent: !chooserItem.lv.tabs ? si : tabstyleloader.item ? tabstyleloader.item.tab : null

                property Item tab: chooserItem
                property bool nextSelected: chooserItem.lv.currentIndex === index + 1
                property bool previousSelected: chooserItem.lv.currentIndex === index - 1
                property string title: ""
                property int count: chooserItem.lv.count

                property Item control: Item {
                    property int tabPosition: Qt.TopEdge
                    property int count: loader.count
                }

                Loader {
                    id: tabloader
                    property Item control: loader.control
                    Component {
                        id: t_
                        TabView {
                            visible: false
                            height: 0
                            width: 0
                        }
                    }
                    sourceComponent: chooserItem.lv.tabs ? t_ : null
                }
                Loader {
                    id: tabstyleloader
                    property Item control: loader.control
                    sourceComponent: tabloader.item ? tabloader.item.style : null
                }

                property int index: model.index

                Label {
                    z: loader.z + 1
                    id: lbl
                    x: chooserItem.hpadding
                    y: chooserItem.vpadding + (chooserItem.lv.tabs * !chooserItem.ListView.isCurrentItem * 2)
                    text: model["display"]
                    color: chooserItem.lv.tabs ? pal.text : chooserItem.ListView.isCurrentItem ? pal.highlightedText : pal.text
                }

                MouseArea {
                    z: lbl.z+1
                    anchors.fill: parent
                    acceptedButtons: Qt.LeftButton
                    onClicked: chooserItem.lv.currentIndex = model.index
                }
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
