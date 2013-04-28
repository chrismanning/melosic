import QtQuick 2.1

import Melosic.Playlist 1.0

ListView {
    id: root
    property int padding: 3

    property PlaylistManager manager

    model: manager.parts.viewer
    orientation: Qt.Horizontal
    currentIndex: manager.currentIndex
    highlightFollowsCurrentItem: true
    highlightMoveDuration: 166
    interactive: false
    boundsBehavior: Flickable.StopAtBounds
}
