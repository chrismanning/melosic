import QtQuick 2.2
import QtQuick.Controls 1.0
import QtQuick.Controls.Private 1.0

MouseArea {
    id: root

    property var text
    property alias interval: timer.interval

    hoverEnabled: true

    onPressed: mouse.accepted = false

    onExited: Tooltip.hideText()
    onCanceled: Tooltip.hideText()

    Timer {
        id: timer
        interval: 1000
        running: root.containsMouse && root.text
        onTriggered: Tooltip.showText(root, Qt.point(root.mouseX, root.mouseY), root.text)
    }
}
