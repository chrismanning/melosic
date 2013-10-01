import QtQuick 2.1

import QtQuick.Controls 1.0
import QtQuick.Controls.Private 1.0
import QtQuick.Controls.Styles 1.0

import Melosic.Playlist 1.0

import "secstomins.js" as SecsToMins

ListView {
    id: root
    property int padding: 3

    property PlaylistManager manager

    model: playlistManagerModel
    delegate: Item {
        id: viewerItem
        width: clv.lv.width
        height: clv.lv.height
        visible: ListView.isCurrentItem
        Connections {
            target: clv.lv
            onCurrentIndexChanged: {
                if(viewerItem.ListView.isCurrentItem) {
                    manager.currentPlaylist = clv
                    manager.currentModel = playlistModel
                }
            }
            onCountChanged: {
                if(viewerItem.ListView.isCurrentItem) {
                    manager.currentPlaylist = clv
                    manager.currentModel = playlistModel
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

    orientation: Qt.Horizontal
    currentIndex: manager.currentIndex
    highlightFollowsCurrentItem: true
    highlightMoveDuration: 0
    interactive: false
    boundsBehavior: Flickable.StopAtBounds
}
