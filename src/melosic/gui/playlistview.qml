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

    focus: true
    Keys.onTabPressed: {
        console.debug("PlaylistView: Tab pressed")
        if(count == 1)
            return
        if(manager.currentIndex + 1 >= count)
            manager.currentIndex = 0
        else
            manager.currentIndex++
    }

    Keys.onBacktabPressed: {
        console.debug("PlaylistView: Shift+Tab pressed")
        if(count == 1)
            return
        if(manager.currentIndex - 1 < 0)
            manager.currentIndex = count-1
        else
            manager.currentIndex--
    }

    MouseArea {
        anchors.fill: parent
        onPressed: {
            root.forceActiveFocus()
            mouse.accepted = false
        }
    }

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
            padding: root.padding
            removeCallback: function(from,count) { return playlistModel.removeRows(from, count) }
            moveCallback: function(from,count,to) { return playlistModel.moveRows(from, count, to) }

            contextMenu: root.contextMenu

            doubleClickAction: function(data) {
                PlayerControls.stop()
                playlistManagerModel.currentPlaylistModel = playlistModel
                console.debug("item in current playlist; jumping & playing")
                PlayerControls.jumpTo(data.index)
                PlayerControls.play()
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
                        ColumnLayout {
                            spacing: clv.padding

                            property string cat_line
                            TagBinding on cat_line {
                                formatString: "%{albumartist} - %{album}"
                            }
                            Label {
                                text: cat_line
                                elide: Text.ElideRight
                                color: textColor
                                Layout.fillWidth: true
                            }

                            property string cat_line2
                            TagBinding on cat_line2 {
                                formatString: "%{genre} | %{date} | "
                            }
                            Label {
                                text: cat_line2 + itemCount + " tracks"
                                elide: Text.ElideRight
                                color: textColor
                                Layout.fillWidth: true
                            }
                        }
                    }
                    Component {
                        id: fileCategoryComponent
                        Column {
                            spacing: clv.padding

                            Label {
                                property var arr: model ? model.filepath.match(pat) : null
                                text: arr ? arr[0] : "NO RESULT"
                                elide: Text.ElideRight
                                color: textColor
                                Layout.fillWidth: true
                                property var pat: new RegExp(pathCriteria.pattern)
                            }
                            Label {
                                text: !model ? "NO MODEL WTF" : model.extension + " | " + itemCount + " tracks"
                                elide: Text.ElideRight
                                color: textColor
                                Layout.fillWidth: true
                            }
                        }
                    }
                    sourceComponent: model && model.tags_readable ? tagCategoryComponent : fileCategoryComponent
                }
            }

            delegate: Loader {
                Component {
                    id: tagComponent
                    RowLayout {
                        id: track
                        spacing: clv.padding
                        Row {
                            Layout.fillWidth: true
                            spacing: parent.spacing
                            Label {
                                id: trackno
                                elide: Text.ElideRight
                                color: textColor
                                width: 15
                                TagBinding on text {
                                    formatString: "%{tracknumber}"
                                }
                            }
                            Label {
                                x: spacing
                                elide: Text.ElideRight
                                color: textColor
                                TagBinding on text {
                                    formatString: "%{title}"
                                }
                            }
                        }
                        Label {
                            id: durationLbl
                            width: contentWidth

                            color: textColor
                            text: SecsToMins.secsToMins(model ? model.duration : 0)
                        }
                    }
                }
                Component {
                    id: fileComponent
                    RowLayout {
                        spacing: clv.padding
                        Label {
                            Layout.fillWidth: true
                            elide: Text.ElideRight
                            color: textColor
                            text: model ? model.file : "NO FILE"
                        }

                        Label {
                            id: durationLbl

                            color: textColor
                            text: SecsToMins.secsToMins(model ? model.duration : 0)
                        }
                    }
                }

                sourceComponent: model && model.tags_readable ? tagComponent : fileComponent
            }
        }
    }

    DropArea {
        anchors.fill: parent
        keys: ["text/uri-list", "melo/filter.selection"]

        onDropped: {
            if(!manager.currentPlaylist)
                return
            if(!manager.currentModel)
                return

            var index = manager.currentPlaylist.indexAt(drop.x + manager.currentPlaylist.flickableItem.contentX,
                                                        drop.y + manager.currentPlaylist.flickableItem.contentY)

            if(drop.hasUrls && drop.proposedAction === Qt.CopyAction) {
                manager.currentModel.insertTracks(index, drop.urls)
                drop.acceptProposedAction()
            }
            if(drop.formats.indexOf("melo/filter.selection" >= 0)) {
                if(drop.source.mimeData !== "undefined" &&
                        drop.source.mimeData["melo/filter.selection"] !== "undefined") {
                    manager.currentModel.insertTracks(index, drop.source.mimeData["melo/filter.selection"])
                    drop.acceptProposedAction()
                }
            }
        }

        onPositionChanged: {

        }
    }

    orientation: Qt.Horizontal
    currentIndex: manager.currentIndex
    highlightFollowsCurrentItem: true
    highlightMoveDuration: 0
    interactive: false
    boundsBehavior: Flickable.StopAtBounds
}
