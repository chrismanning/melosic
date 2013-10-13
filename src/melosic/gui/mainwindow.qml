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
            MenuItem { action: refreshTracksAction }
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

    Action {
        id: refreshTracksAction
        text: "Refresh Tags of selected/all"
        onTriggered: {
            var pm = playlistManager.currentModel
            if(pm == null)
                return
            if(currentPlaylist.selected.count <= 0) {
                pm.refreshTags(0,-1)
                return
            }

            var groups = currentPlaylist.contiguousSelected().reverse()
            for(var i = 0; i < groups.length; i++) {
                var g = groups[i]
                if(g.length <= 0)
                    continue
                pm.refreshTags(g[0], g[g.length-1])
            }
        }
    }

    property alias currentPlaylist: playlistManager.currentPlaylist

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
                property var dur: currentPlaylist.reduce(function(previousValue, currentValue, index, array){
                    return parseInt(previousValue) + parseInt(currentValue.duration);
                }, currentPlaylist.count > 0 ? 0 : 0)
                text: '[' + SecsToMins.secsToMins(dur) + ']'
            }
            Label {
                text: playerControls.stateStr
            }
        }
    }

    ConfigDialog {
        id: configDialog
        modality: Qt.WindowModal
    }

    SystemPalette {
        id: pal
    }

    Component.onCompleted: {
        if(!playlistManagerModel.rowCount())
            playlistManagerModel.insertRows(playlistManagerModel.rowCount(),1)
        playlistManager.currentIndex = 0
    }

    PlaylistManager {
        id: playlistManager
    }

    SplitView {
        anchors.fill: parent
        orientation: Qt.Horizontal

        PlaylistChooser {
            id: playlistChooser
            Layout.minimumWidth: 100
            Layout.alignment: Qt.AlignLeft
            background: pal.base
            manager: playlistManager
            orientation: Qt.Vertical
        }

        ColumnLayout {
            Layout.fillHeight: true
            Layout.fillWidth: true
            spacing: 0
            RowLayout {
                spacing: 0

                PlaylistChooser {
                    id: playlistTabs
                    Layout.preferredHeight: 28
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignBottom
                    tabs: true
                    spacing: -2
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
                id: playlistView
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.minimumWidth: 100
                Layout.minimumHeight: 100
                manager: playlistManager
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
                var pm = playlistManager.currentModel
                if(pm !== null) {
                    pm.insertTracks(currentPlaylist.currentIndex, fileDialog.fileUrls)
                    currentPlaylist.currentIndex += fileDialog.fileUrls.length
                }
                else {
                    console.debug("Cannot insert tracks: invalid playlist")
                }
            }
        }
    }
}
