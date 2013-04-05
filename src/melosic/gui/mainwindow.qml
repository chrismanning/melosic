import QtQuick 2.1
import QtQuick.Dialogs 1.0
import QtQuick.Controls 1.0
import QtQuick.Controls.Private 1.0

//import "qrc:/qml/";
import Melosic.Playlist 1.0

ApplicationWindow {
    id: mainWindow
    width: 600
    height: 500
    title: "Melosic"

    menuBar: MenuBar {
        Menu {
            title: "File"
            MenuItem { action: openAction }
            MenuItem { action: saveAction }
            MenuSeparator {}
            MenuItem { action: quitAction }
        }
        Menu {
            title: "Edit"
            MenuItem { action: optionsAction }
        }
        Menu {
            title: "Playback"
            MenuItem { action: playAction }
            MenuItem { action: pauseAction }
            MenuItem { action: previousAction }
            MenuItem { action: nextAction }
            MenuItem { action: stopAction }
        }
        Menu {
            title: "Playlist"
            MenuItem { action: selectAllAction }
            MenuItem { action: clearSelectionAction }
        }
    }

    Action {
        id: openAction
        text: "Open"
        shortcut: "ctrl+o"
        onTriggered: {
//            playlistModel.appendFiles(["file://home/chris/Music/02 - Sorgmantel.flac"])
            fileDialog.visible = !fileDialog.visible
        }
    }
    Action {
        id: saveAction
        text: "Save Playlist"
        shortcut: "ctrl+s"
    }
    Action {
        id: quitAction
        text: "Quit"
        shortcut: "ctrl+q"
        onTriggered: Qt.quit()
    }

    Action {
        id: optionsAction
        text: "Options"
        onTriggered: {
            configDialog.visible = !configDialog.visible
        }
    }

    Action {
        id: playAction
        text: "Play"
        onTriggered: playerControls.play()
    }
    Action {
        id: pauseAction
        text: "Pause"
        onTriggered: playerControls.pause()
    }
    Action {
        id: previousAction
        text: "Previous"
        onTriggered: playerControls.previous()
    }
    Action {
        id: nextAction
        text: "Next"
        onTriggered: playerControls.next()
    }
    Action {
        id: stopAction
        text: "Stop"
        onTriggered: playerControls.stop()
    }

    Action {
        id: selectAllAction
        text: "Select All"
        shortcut: "ctrl+a"
        onTriggered: playlist.selectAll()
    }
    Action {
        id: clearSelectionAction
        text: "Clear Selection"
        onTriggered: playlist.clearSelection()
    }

    statusBar: StatusBar {
        id: status
        property var playlistModel
        Binding on playlistModel {
            value: playlistManagerModel.playlist(playlistChooser.currentItem.myData.display)
        }

        Row {
            anchors.verticalCenter: parent.verticalCenter

            Label {
                text: "" + playlist.count + " tracks"
            }
        }
    }

    ConfigDialog {
        id: configDialog
        modality: Qt.WindowModal
    }

    SystemPalette { id: pal }

    Component.onCompleted: {
        if(!playlistManagerModel.rowCount())
            playlistManagerModel.insertRows(playlistManagerModel.rowCount(),1)
        playlistChooser.currentIndex = 0
    }

    Column {
        anchors.fill: parent
        Row {
            Rectangle {
                width: 100
                height: 50
                color: pal.light
                ListView {
                    id: playlistChooser
                    model: playlistManagerModel
                    focus: true
                    anchors.fill: parent
                    delegate: Item {
                        property var myData: model
                        height: name.height
                        width: playlistChooser.width

                        Row {
                            anchors.fill: parent
                            Text {
                                id: name
                                renderType: Text.NativeRendering
                                elide: Text.ElideRight
                                text: display
                                color: parent.parent.ListView.isCurrentItem ? pal.highlightedText : pal.text
                            }
                        }
                        MouseArea {
                            anchors.fill: parent
                            acceptedButtons: Qt.LeftButton
                            onClicked: playlistChooser.currentIndex = model.index
                        }
                    }
                    highlightFollowsCurrentItem: true
                    highlight: StyleItem {
                        elementType: "itemrow"
                        selected: true
                        active: playlistChooser.activeFocus
                    }

                    onCurrentIndexChanged: {
                        if(playlistManagerModel.rowCount() > 0 && currentIndex !== -1)
                            playlist.model = playlistManagerModel.playlist(playlistChooser.currentItem.myData.display)
                    }
                }
            }
            Button {
                text: "Add"
                onClicked: playlistManagerModel.insertRows(playlistManagerModel.rowCount(),1)
            }
        }

        CategoryListView {
            id: playlist
            anchors.left: parent.left
            anchors.right: parent.right
            frameVisible: true
            focus: true
            spacing: 3
            height: parent.height - playlistChooser.height+1

            category: Category {
                CategoryRole { role: "artist" }
                CategoryRole { role: "album" }

                delegate: Column {
                    spacing: playlist.spacing

                    y: spacing
                    x: spacing
                    height: childrenRect.height + spacing*(children.length)
                    Text {
                        text: model.artist + " - " + model.album
                        elide: Text.ElideRight
                        color: textColor
                        renderType: Text.NativeRendering
                    }
                    Text {
                        text: model.genre + " | " + model.year + " | " + itemCount + " tracks"
                        elide: Text.ElideRight
                        color: textColor
                        renderType: Text.NativeRendering
                    }
                }
            }

            delegate: Row {
                id: track
                spacing: playlist.spacing
                x: spacing
                Row {
                    spacing: playlist.spacing
                    width: itemWidth - duration.width - spacing*(track.children.length+1)
                    Text {
                        renderType: Text.NativeRendering
                        elide: Text.ElideRight
                        color: textColor
                        text: model.tracknumber
                        width: 15
                    }
                    Text {
                        renderType: Text.NativeRendering
                        elide: Text.ElideRight
                        color: textColor
                        text: model.title
                    }
                }
                Text {
                    id: duration
                    renderType: Text.NativeRendering
                    width: contentWidth

                    color: textColor
                    text: Math.floor(model.length/60) + ":" + (model.length%60 > 9 ? "" : "0") + model.length%60
                }
            }
        }
    }

    FileDialog {
        id: fileDialog
        title: "Choose some files"
        selectMultiple: true
        nameFilters: [ "Audio files (*.flac)", "All files (*)" ]
        selectedNameFilter: "Audio files (*.flac)"

        onAccepted: {
            if(fileDialog.fileUrls.length) {
                var pm = playlistManagerModel.playlist(playlistChooser.currentItem.myData.display)
                if(pm !== null) {
                    pm.insertTracks(playlist.currentIndex, fileDialog.fileUrls)
                    playlist.currentIndex += fileDialog.fileUrls.length
                }
                else {
                    console.debug("Cannot insert tracks: invalid playlist")
                }
            }
        }
//        modality: Qt.NonModal
    }
}
