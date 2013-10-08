import QtQuick 2.1
import QtQuick.Controls 1.1

Rectangle {
    id: tooltip
    width: txt.width + 20
    height: txt.height + 10
    radius: 4

    property int fadeInDelay
    property int fadeOutDelay

    property alias text: txt.text

    color: pal.text
    border.color: pal.highlightedText
    border.width: 1
    state: ""

    SystemPalette {
        id: pal
    }

    Label {
        id: txt
        color: pal.highlightedText
        anchors.centerIn: parent
    }

    states: [
        State {
            name: "poppedUp"
            PropertyChanges { target: tooltip; opacity: 1 }
        },

        State {
            name: "poppedDown"
            PropertyChanges { target: tooltip; opacity: 0 }
        }
    ]

    transitions: [
        Transition {
            from: ""
            to: "poppedUp"
            PropertyAnimation { target: tooltip; property: "opacity"; duration: tooltip.fadeInDelay; }
        },

        Transition {
            from: "poppedUp"
            to: "poppedDown"
            PropertyAnimation { target: tooltip; property: "opacity"; duration: tooltip.fadeOutDelay; }
        }
    ]
}
