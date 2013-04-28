import QtQuick 2.1
import QtQml.Models 2.1

import QtQuick.Controls 1.0
import QtQuick.Controls.Private 1.0

import Melosic.Playlist 1.0

DelegateModel {
    property CategoryListView currentPlaylist
    property QtObject currentModel
    property int currentIndex

    model: playlistManagerModel

    delegate: Package {
        Item {
            Package.name: 'chooser'
            id: chooserItem

            property int padding: 3
            height: lbl.height
            width: si.horizontal ? (padding * 2) + lbl.width : parent.width

            StyleItem {
                id: si
                anchors.fill: parent
                elementType: "itemrow"
                horizontal: lv.orientation === Qt.Horizontal
                property var lv: chooserItem.ListView.view

                Label {
                    x: chooserItem.padding
                    id: lbl
                    text: model["display"]
                    color: si.selected ? pal.highlightedText : pal.text
                }
                selected: chooserItem.ListView.isCurrentItem

                MouseArea {
                    z: 1
                    anchors.fill: parent
                    acceptedButtons: Qt.LeftButton
                    onClicked: si.lv.currentIndex = model.index
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
                        width: itemWidth - duration.width - (spacing*3)
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
                        id: duration
                        width: contentWidth

                        color: textColor
                        text: Math.floor(model.length/60) + ":" + (model.length%60 > 9 ? "" : "0") + model.length%60
                    }
                }
            }
        }
    }
}
