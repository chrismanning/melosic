import QtQuick 2.2

import Melosic.Playlist 1.0

Item {
    id: root
    property CategoryListView currentPlaylist
    property QtObject currentModel
    Connections {
        target: playlistManagerModel
        onCurrentPlaylistModelChanged: {
            currentModel = playlistManagerModel.currentPlaylistModel
        }
    }
    onCurrentModelChanged: {
        if(currentModel === playlistManagerModel.currentPlaylistModel)
            return
        if(PlayerControls.state !== PlayerControls.Playing && PlayerControls.state !== PlayerControls.Paused)
            playlistManagerModel.currentPlaylistModel = currentModel
    }

    property int currentIndex
}
