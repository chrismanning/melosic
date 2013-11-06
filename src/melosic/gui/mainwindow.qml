import QtQuick 2.2
import QtQuick.Dialogs 1.0
import QtQuick.Controls 1.0
import QtQuick.Layouts 1.0
import QtQuick.Window 2.1

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
            title: "View"
            MenuItem { action: optionsAction }
            MenuItem { action: logWindowAction }
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
        shortcut: StandardKey.Open
        iconName: "document-open"
        onTriggered: {
            fileDialog.visible = !fileDialog.visible
        }
    }
    Action {
        id: saveAction
        text: "Save Playlist"
        shortcut: StandardKey.Save
        iconName: "document-save"
    }
    Action {
        id: quitAction
        text: "Quit"
        shortcut: StandardKey.Quit
        iconName: "application-exit"
        onTriggered: {
            playerControls.stop()
            Qt.quit()
        }
    }

    Action {
        id: optionsAction
        text: "Options"
        shortcut: StandardKey.Preferences
        iconName: "configure"
        onTriggered: {
            configDialog.visible = !configDialog.visible
        }
    }
    Action {
        id: logWindowAction
        text: "View Log"
        onTriggered: {
            if(enable_logging)
                logWindow.visible = !logWindow.visible
        }
    }

    Action {
        id: playAction
        text: "Play"
        iconName: "media-playback-start"
        onTriggered: playerControls.play()
    }
    Action {
        id: pauseAction
        text: "Pause"
        iconName: "media-playback-pause"
        onTriggered: playerControls.pause()
    }
    Action {
        id: previousAction
        text: "Previous"
        iconName: "media-skip-backward"
        onTriggered: playerControls.previous()
    }
    Action {
        id: nextAction
        text: "Next"
        iconName: "media-skip-forward"
        onTriggered: playerControls.next()
    }
    Action {
        id: stopAction
        text: "Stop"
        iconName: "media-playback-stop"
        onTriggered: playerControls.stop()
    }

    Action {
        id: selectAllAction
        text: "Select All"
        shortcut: StandardKey.SelectAll
        iconName: "edit-select-all"
        onTriggered: currentPlaylist.selectAll()
    }
    Action {
        id: clearSelectionAction
        text: "Clear Selection"
        shortcut: StandardKey.Deselect
        iconName: "edit-clear"
        onTriggered: currentPlaylist.clearSelection()
    }
    Action {
        id: removeTracksAction
        text: "Remove Selected"
        shortcut: StandardKey.Delete
        iconName: "edit-delete"
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
        shortcut: StandardKey.Refresh
        iconName: "view-refresh"
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

                text: (selected ? currentPlaylist.selected.count : count)
                      + " tracks" + (selected ? " selected" : "")
            }
            Label {
                id: playlistDuration
                text: '[' + SecsToMins.secsToMins(playlistManagerModel.currentPlaylistModel.duration) + ']'
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

    property var logWindow: logWindowLoader.item
    Loader {
        id: logWindowLoader
        active: enable_logging
        sourceComponent: Window {
            modality: Qt.NonModal
            width: 500
            height: 500
            visible: false
            TextArea {
                id: logText
                anchors.fill: parent
                readOnly: true
                textFormat: TextEdit.PlainText
            }
            Component.onCompleted: {
                logsink.textEdit = logText
            }
        }
    }

    Component.onCompleted: {
        if(!playlistManagerModel.rowCount())
            playlistManagerModel.insertRows(playlistManagerModel.rowCount(),1)
        playlistManager.currentIndex = 0
    }

    PlaylistManager {
        id: playlistManager
    }

    minimumHeight: 400
    minimumWidth: 400

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
            Layout.minimumWidth: 100
            Layout.minimumHeight: 100
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
