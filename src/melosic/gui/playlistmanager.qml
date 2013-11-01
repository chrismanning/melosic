import QtQuick 2.1

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

    property int currentIndex
}
