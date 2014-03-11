import QtQuick 2.2
import QtQuick.Dialogs 1.1
import QtQuick.Controls 1.1
import QtQuick.Controls.Private 1.0
import QtQuick.Layouts 1.1
import QtQuick.Window 2.1

import Melosic.Playlist 1.0
import Melosic.Browser 1.0

import "secstomins.js" as SecsToMins
import "Log.js" as Log

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
            MenuSeparator {}
            MenuItem { action: jumpPlayAction }
        }
        Menu {
            title: "Playlist"
            MenuItem { action: selectAllAction }
            MenuItem { action: clearSelectionAction }
            MenuSeparator {}
            MenuItem { action: removeTracksAction }
            MenuItem { action: refreshTracksAction }
            MenuSeparator {}
            MenuItem { action: addPlaylistAction }
            MenuItem { action: removePlaylistAction }
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
            PlayerControls.stop()
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
        onTriggered: PlayerControls.play()
    }
    Action {
        id: pauseAction
        text: "Pause"
        iconName: "media-playback-pause"
        onTriggered: PlayerControls.pause()
    }
    Action {
        id: previousAction
        text: "Previous"
        iconName: "media-skip-backward"
        onTriggered: PlayerControls.previous()
    }
    Action {
        id: nextAction
        text: "Next"
        iconName: "media-skip-forward"
        onTriggered: PlayerControls.next()
    }
    Action {
        id: stopAction
        text: "Stop"
        iconName: "media-playback-stop"
        onTriggered: PlayerControls.stop()
    }
    Action {
        id: jumpPlayAction
        text: "Jump to current and play"
        iconName: "go-jump"
        onTriggered: {
            PlayerControls.stop()
            playlistManagerModel.currentPlaylistModel = playlistManager.currentModel
            console.debug("item in current playlist; jumping & playing")
            PlayerControls.jumpTo(playlistManager.currentPlaylist.currentIndex)
            PlayerControls.play()
        }
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

    Action {
        id: addPlaylistAction
        text: "Add New Playlist"
        iconName: "list-add"
        onTriggered: playlistManagerModel.insertRows(playlistManagerModel.rowCount(),1)
    }
    Action {
        id: removePlaylistAction
        text: "Remove Current Playlist"
        iconName: "list-remove"
        onTriggered: playlistManagerModel.removeRows(playlistManager.currentIndex,1)
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
                text: PlayerControls.stateStr
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

    Action {
        id: appendToPlaylist
        text: "Append to current playlist"
        onTriggered: {
            var pm = playlistManager.currentModel
            if(pm !== null)
                pm.insertTracks(currentPlaylist.currentIndex, filterView.filterResultPane.model)
        }
    }

    SplitView {
        anchors.fill: parent
        orientation: Qt.Horizontal

        FilterView {
            id: filterView
            Layout.minimumWidth: 100
            Layout.alignment: Qt.AlignLeft
            onActivated: {
                console.debug("FilterView.activated")
                var pm = playlistManager.currentModel
                if(pm !== null)
                    pm.insertTracks(currentPlaylist.currentIndex, model)
            }

            FilterPane {
                id: genre
                objectName: "genrepane"

                Component.onCompleted: {
                    query = {}
                    paths = {
                        genre: "metadata[?(@.key == 'genre')].value"
                    }
                }
                delegate: Label {
                    x: 1
                    text: document.genre === undefined ? "Unknown Genre" : document.genre
                    color: styleData.textColor
                }

                header: "Genre"
            }
            FilterPane {
                id: artist
                dependsOn: genre
                dependsPath: "$.genre"
                objectName: "artistpane"

                delegate: Label {
                    x: 1
                    text: document.artist === undefined ? "Unknown Artist" : document.artist
                    color: styleData.textColor
                }

                function init_query() {
                    var tmp = {
                        metadata: {
                            $elemMatch: {
                                key: "genre",
                                value: {
                                    $in: dependSelection
                                }
                            }
                        }
                    }

                    var has_unknowns = dependSelection.some(function(v) {return v === ""})

                    if(has_unknowns)
                        tmp.metadata.$elemMatch.value.$in = dependSelection.filter(function(v) {return v !== ""})
                    if(tmp.metadata.$elemMatch.value.$in.length === 0)
                        delete tmp.metadata

                    if(has_unknowns) {
                        var u_qry = //{ $or:[//TODO: negate an elemMatch when it's supported
//                                { metadata: {$not: {$elemMatch: {key: "genre"} } } },
                                {'metadata.0': { $exists: false } }
                            //]}
                        if(tmp.metadata === undefined)
                            tmp = u_qry
                        else
                            tmp = { $or: [tmp, u_qry] }
                    }

                    query = tmp
                }

                Component.onCompleted: {
                    init_query()
                    paths = {
                        artist: "metadata[?(@.key == 'artist')].value"
                    }
                }
                onDependSelectionChanged: init_query()
            }
            FilterPane {
                id: album
                dependsOn: artist
                dependsPath: "$.artist"
                objectName: "albumpane"

                sortFields: ["date", "year", "album"]

                delegate: Row {
                    x: 1
                    spacing: 5
                    Label {
                        width: 30
                        visible: document.album !== undefined
                        text: document.date === undefined ? "0000" : document.date
                        color: styleData.textColor
                    }
                    Label {
                        text: document.album === undefined ? "Unknown Album" : document.album
                        color: styleData.textColor
                    }
                }

                function init_query() {
                    var tmp = {
                        $and: [
                            { metadata: { $elemMatch: { key: "artist", value: { $in: dependSelection } } } }
                        ]
                    }

                    if(dependsOn !== undefined && dependsOn.dependSelection.length > 0 && dependsOn.query.length !== 0)
                        tmp.$and.push(dependsOn.query)

                    var has_unknowns = dependSelection.some(function(v) {return v === ""})

                    if(has_unknowns)
                        tmp.$and[0].metadata.$elemMatch.value.$in = dependSelection.filter(function(v) {return v !== ""})
                    if(tmp.$and[0].metadata.$elemMatch.value.$in.length === 0)
                        tmp.$and.splice(0, 1)

                    if(tmp.$and.length === 0)
                        delete tmp.$and
                    else if(tmp.$and.length === 1)
                        tmp = tmp.$and[0]

                    if(has_unknowns) {
                        var u_qry = //{ $or:[//TODO: negate an elemMatch when it's supported
//                                { metadata: {$not: {$elemMatch: {key: "genre"} } } },
                                {'metadata.0': { $exists: false } }
                            //]}
                        if(tmp.$and === undefined && tmp.metadata === undefined)
                            tmp = u_qry
                        else
                            tmp = { $or: [tmp, u_qry] }
                    }

                    query = tmp
                }

                Component.onCompleted: {
                    init_query()
                    paths = {
                        album: "metadata[?(@.key == 'album')].value",
                        date: "metadata[?(@.key == 'date')].value"
                    }
                }
                onDependSelectionChanged: init_query()
            }
            Connections {
                target: album.dependsOn
                onDependSelectionChanged: album.init_query()
            }

            filterResultPane: track
            FilterPane {
                id: track
                dependsOn: album
                dependsPath: "$.album"
                objectName: "trackpane"

                sortFields: ["tracknumber", "title"]

                delegate: Row {
                    x: 1
                    spacing: 5
                    Label {
                        visible: document.title !== undefined
                        text: document.tracknumber === undefined ? "00" : document.tracknumber
                        color: styleData.textColor
                    }
                    Label {
                        text: document.title === undefined ? document.location : document.title
                        color: styleData.textColor
                    }
                }

                function init_query() {
                    var tmp = {
                        $and: [
                            { metadata: { $elemMatch: { key: "album", value: { $in: dependSelection } } } }
                        ]
                    }

                    if(dependsOn && dependsOn.dependsOn !== undefined && dependsOn.dependsOn.dependSelection.length > 0) {
                        if(dependsOn.dependsOn.query.$and !== undefined)
                            tmp.$and = tmp.$and.concat(dependsOn.dependsOn.query.$and)
                        else
                            tmp.$and.push(dependsOn.dependsOn.query)
                    }

                    if(dependsOn !== undefined && dependsOn.dependSelection.length > 0) {
                        if(dependsOn.query.$and !== undefined)
                            tmp.$and = tmp.$and.concat(dependsOn.query.$and)
                        else
                            tmp.$and.push(dependsOn.query)
                    }

                    var has_unknowns = dependSelection.some(function(v) {return v === ""})

                    if(has_unknowns)
                        tmp.$and[0].metadata.$elemMatch.value.$in = dependSelection.filter(function(v) {return v !== ""})
                    if(tmp.$and[0].metadata.$elemMatch.value.$in.length === 0)
                        tmp.$and.splice(0, 1)

                    if(tmp.$and.length === 0)
                        delete tmp.$and
                    else if(tmp.$and.length === 1)
                        tmp = tmp.$and[0]

                    if(has_unknowns) {
                        var u_qry = //{ $or:[//TODO: negate an elemMatch when it's supported
//                                { metadata: {$not: {$elemMatch: {key: "genre"} } } },
                                {'metadata.0': { $exists: false } }
                            //]}
                        if(tmp.$and === undefined && tmp.metadata === undefined)
                            tmp = u_qry
                        else
                            tmp = { $or: [tmp, u_qry] }
                    }

                    query = tmp
                }

                Component.onCompleted: {
                    init_query()
                    paths = {
                        title: "metadata[?(@.key == 'title')].value",
                        tracknumber: "metadata[?(@.key == 'tracknumber')].value",
                        location: "location"
                    }
                }
                onDependSelectionChanged: init_query()
            }
            Connections {
                target: track.dependsOn
                onDependSelectionChanged: track.init_query()
            }
            Connections {
                target: track.dependsOn.dependsOn
                onDependSelectionChanged: track.init_query()
            }
        }

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

                ToolButton {
                    z: 1
                    action: addPlaylistAction
                }
            }

            PlaylistView {
                id: playlistView
                Layout.fillWidth: true
                Layout.fillHeight: true
                manager: playlistManager

                contextMenu: Menu {
                    MenuItem { action: jumpPlayAction }
                    MenuItem { action: removeTracksAction }
                }
            }

            RowLayout {
                id: buttonRow
                Layout.alignment: Qt.AlignHCenter
                ToolButton { action: playAction }
                ToolButton { action: pauseAction }
                ToolButton { action: stopAction }
                ToolButton { action: previousAction }
                ToolButton { action: nextAction }
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
