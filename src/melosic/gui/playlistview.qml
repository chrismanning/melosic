import QtQuick 2.2

import QtQuick.Controls 1.1
import QtQuick.Controls.Private 1.0
import QtQuick.Controls.Styles 1.1
import QtQuick.Layouts 1.1

import Melosic.Playlist 1.0

import "secstomins.js" as SecsToMins

ListView {
    id: root
    property int padding: 3

    property PlaylistManager manager

    property Component contextMenu

    model: playlistManagerModel
    delegate: Item {
        id: viewerItem
        width: root.width
        height: root.height
        visible: ListView.isCurrentItem
        Connections {
            target: root
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
            anchors.fill: parent
            model: playlistModel
            frameVisible: true
            focus: viewerItem.ListView.isCurrentItem
            padding: 3
            removeCallback: function(from,count) { return playlistModel.removeRows(from, count) }
            moveCallback: function(from,count,to) { return playlistModel.moveRows(from, count, to) }

            contextMenu: root.contextMenu

            doubleClickAction: function(data) {
                playerControls.stop()
                playlistManagerModel.currentPlaylistModel = playlistModel
                console.debug("item in current playlist; jumping & playing")
                playerControls.jumpTo(data.index)
                playerControls.play()
            }

            category: Category {
                id: __category

                CategoryRole {
                    id: pathCriteria
                    role: "filepath"
                    pattern: ".*(?=/.*)"
                }
                CategoryTag { field: "albumartist" }
                CategoryTag { field: "album" }
                CategoryTag { field: "date" }

                delegate: Loader {
                    Component {
                        id: tagCategoryComponent
                        Column {
                            spacing: root.padding

                            y: spacing
                            x: spacing
                            height: childrenRect.height + spacing*(children.length)
                            width: root.width

                            property string artist
                            TagBinding on artist {
                                formatString: "artist"
                            }
                            property string album
                            TagBinding on album {
                                formatString: "album"
                            }
                            Label {
                                text: artist + " - " + album
                                elide: Text.ElideRight
                                color: textColor
                                width: parent.width
                            }

                            property string genre
                            TagBinding on genre {
                                formatString: "genre"
                            }
                            property string year
                            TagBinding on year {
                                formatString: "date"
                            }
                            Label {
                                text: genre + " | " + year + " | " + itemCount + " tracks"
                                elide: Text.ElideRight
                                color: textColor
                                width: parent.width
                            }
                        }
                    }
                    Component {
                        id: fileCategoryComponent
                        Column {
                            spacing: root.padding

                            y: spacing
                            x: spacing
                            height: childrenRect.height + spacing*(children.length)
                            width: root.width
                            Label {
                                property var arr: model.filepath.match(pat)
                                text: arr ? arr[0] : ""
                                elide: Text.ElideRight
                                color: textColor
                                width: parent.width
                                property var pat: new RegExp(pathCriteria.pattern)
                            }
                            Label {
                                text: model.extension + " | " + itemCount + " tracks"
                                elide: Text.ElideRight
                                color: textColor
                                width: parent.width
                            }
                        }
                    }
                    sourceComponent: model.tags_readable ? tagCategoryComponent : fileCategoryComponent
                }
            }

            tooltip: ToolTip {
                width: rowId.width + 20
                height: rowId.height + 10
                radius: 3
                fadeInDelay: 500
                fadeOutDelay: 700

                RowLayout {
                    id: rowId
                    spacing: 10
                    x: spacing
                    anchors.centerIn: parent

                    Rectangle {
                        height: 10
                        width: 10
                        color: "red"
                    }

                    Label {
                        id: txt
                        color: pal.highlightedText
                        property string title
                        TagBinding on title {
                            formatString: "title"
                        }
                        text: model.tags_readable ? title : model.filepath
                    }
                }
            }

            delegate: Loader {
                Component {
                    id: tagComponent
                    Row {
                        id: track
                        spacing: root.padding
                        x: spacing
                        Row {
                            spacing: parent.spacing
                            width: itemWidth - durationLbl.width - (spacing*3)
                            Label {
                                id: trackno
                                elide: Text.ElideRight
                                color: textColor
                                width: 15
                                TagBinding on text {
                                    formatString: "tracknumber"
                                }
                            }
                            Label {
                                x: spacing
                                elide: Text.ElideRight
                                color: textColor
                                TagBinding on text {
                                    formatString: "title"
                                }
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
                Component {
                    id: fileComponent
                    Label {
                        x: padding
                        elide: Text.ElideRight
                        color: textColor
                        text: model.file
                    }
                }

                sourceComponent: model.tags_readable ? tagComponent : fileComponent
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
