import QtQuick 2.1
import QtQuick.Dialogs 1.0
import QtQuick.Controls 1.0
import QtQuick.Layouts 1.0

import Melosic.Playlist 1.0

import "secstomins.js" as SecsToMins

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
            MenuSeparator {}
            MenuItem { action: removeTracksAction }
        }
    }

    Action {
        id: openAction
        text: "Open"
        shortcut: "ctrl+o"
        onTriggered: {
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
        onTriggered: currentPlaylist.selectAll()
    }
    Action {
        id: clearSelectionAction
        text: "Clear Selection"
        onTriggered: currentPlaylist.clearSelection()
    }
    Action {
        id: removeTracksAction
        text: "Remove Selected"
        shortcut: "del"
        onTriggered: {
            if(currentPlaylist !== null)
                currentPlaylist.removeSelected()
            else
                console.debug("Cannot remove tracks: invalid playlist")
        }
    }

    property var currentPlaylist: playlistManager.currentPlaylist

    statusBar: StatusBar {
        id: status

        Row {
            anchors.verticalCenter: parent.verticalCenter
            spacing: 5

            Label {
                property bool selected: currentPlaylist.selected.count > 0
                property int count: currentPlaylist.count

                text: (selected ? currentPlaylist.selected.count : count) + " tracks" + (selected ? " selected" : "")
            }
            Label {
                id: playlistDuration
                Connections {
                    target: currentPlaylist
                    onCountChanged: {
                        var dur = currentPlaylist.reduce(function(previousValue, currentValue, index, array){
                            return previousValue + currentValue.duration;
                        }, 0)
                        playlistDuration.text = '[' + SecsToMins.secsToMins(dur) + ']'
                    }
                }
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

    PlaylistManager {
        id: playlistManager
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: -1
        RowLayout {
            Layout.preferredHeight: addbtn.height

            PlaylistChooser {
                Layout.fillWidth: true
                id: playlistChooser
                Layout.preferredHeight: addbtn.height
                Layout.preferredWidth: 200
                tabs: true
                padding: 15
                manager: playlistManager
                orientation: Qt.Horizontal
            }

            Button {
                id: addbtn
                z: 1
                text: "Add"
                onClicked: playlistManagerModel.insertRows(playlistManagerModel.rowCount(),1)
            }
        }

        PlaylistView {
            id: playlist
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.preferredWidth: 100
            Layout.preferredHeight: 100
            manager: playlistManager
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
                var pm = playlistManager.currentModel
                if(pm !== null) {
                    pm.insertTracks(playlist.currentIndex, fileDialog.fileUrls)
                    currentPlaylist.currentIndex += fileDialog.fileUrls.length
                }
                else {
                    console.debug("Cannot insert tracks: invalid playlist")
                }
            }
        }
    }
}
