import QtQml.Models 2.1
import QtQuick 2.2
import QtQuick.Controls 1.1
import QtQuick.Controls.Private 1.0

import Melosic.Playlist 1.0
import "naturalsort.js" as NaturalSort

ScrollView {
    id: root
    property alias frameVisible: root.frameVisible

    property var model

    property Component delegate
    property alias currentItem: listView.currentItem
    property alias currentIndex: listView.currentIndex
    property alias count: listView.count
    property int padding: 0
    property int itemHeight: 14
    property var removeItems
    property var moveItems
    property Component tooltip
    property Component categoryTooltip
    property Component contextMenu: null

    property DelegateModelGroup selected: selectedGroup

    property CategoryProxyModel categoryModel: typeof root.model == 'CategoryProxyModel' ? root.model : proxyLoader.item

    property Category category
    Component.onCompleted: {
        categoryModel.category = category
    }

    Keys.onUpPressed: {
        listView.decrementCurrentIndex()
    }
    Keys.onDownPressed: {
        listView.incrementCurrentIndex()
    }

    function indexAt(x, y) {
        return listView.indexAt(x, y)
    }

    function clearSelection() {
        console.debug("clearing selection; count: ", count)
        if(count)
            delegateModel.deselect(0, count)
        currentIndex = -1
    }

    function selectAll() {
        console.debug("selecting all")
        if(count)
            delegateModel.select(0, count)
    }

    property var removeCallback
    function removeSelected() {
        if(removeCallback === null || selected.count <= 0)
            return
        console.debug("removing selected")

        var groups = contiguousSelected().reverse()
        for(var i = 0; i < groups.length; i++) {
            console.debug("i: ",i)
            var g = groups[i]
            if(g.length <= 0)
                continue
            removeCallback(g[0], g[g.length-1] - g[0] + 1)
        }
    }

    property var moveCallback
    function moveSelected(to) {
        if(moveCallback === null || selected.count <= 0)
            return

        var groups = contiguousSelected()
        for(var i = 0; i < groups.length; i++) {
            var g = groups[i]
            if(g.length <= 0)
                continue
            if(i > 0)
                to += g[0] - groups[i-1][0]

            moveCallback(g[0], g[g.length-1] - g[0] + 1, to)
        }
    }

    function contiguousSelected() {
        var groups = new Array

        if(selected.count <= 0)
            return groups

        groups.push(new Array)

        for(var i = 0, j = 0; i < selected.count; i++) {
            if(groups[j].length > 0 && groups[j][groups[j].length-1]+1 !== selected.get(i).itemsIndex) {
                groups.push(new Array)
                groups[j].naturalSort()
                ++j
            }
            groups[j].push(selected.get(i).itemsIndex)
        }

        return groups
    }

    function modelIndex(index) {
        return delegateModel.modelIndex(index)
    }

    function reduce(callback, initialValue) {
        if('function' !== typeof callback)
            throw new TypeError(callback + ' is not a function')
        if(initialValue === undefined)
            throw new TypeError('initialValue required')

        var length = count >>> 0, value = initialValue

        for(var index = 0; length > index; ++index)
            value = callback(value, delegateModel.items.get(index).model, index, this)

        return value
    }

    property var doubleClickAction: null

    ListView {
        id: listView
        anchors.fill: parent
        interactive: false
        boundsBehavior: Flickable.StopAtBounds
        Component.onDestruction: console.debug("Playlist view being destroyed")

        Loader {
            id: proxyLoader

            Component {
                id: categoryModelComponent
                CategoryProxyModel {
                    sourceModel: root.model
                }
            }

            sourceComponent: typeof root.model == 'CategoryProxyModel' ?  undefined : categoryModelComponent
        }

        Loader {
            id: menuLoader
            sourceComponent: contextMenu
        }
        property alias __contextMenu: menuLoader.item

        model: DelegateModel {
            id: delegateModel
            groups: [ DelegateModelGroup { id: selectedGroup; name: "selected" } ]

            model: root.categoryModel

            delegate: Item {
                id: rowitem
                x: root.padding
                z: -index
                width: listView.width - (x*2)
                property bool drawCategory: category && block && block.firstRow === index

                property int baseHeight: category && block && block.collapsed ? 0 : itemHeight

                height: baseHeight + categoryItemLoader.height
                ListView.onRemove: {
                    console.debug("ListView.onRemove: removing item")
                    itemDelegateLoader.sourceComponent = null
                    itemDelegateLoader.active = false
                    categoryItemLoader.sourceComponent = null
                    categoryItemLoader.active = false
                    block = null
                    baseHeight = 0
                }

                property int rowIndex: model.index
                property var itemModel: model
                property CategoryProxyModel categoryModel: root.categoryModel
                property Block block: CategoryProxyModel.block

                Item {
                    id: itemitem
                    anchors.top: categoryItemLoader.bottom
                    height: parent.baseHeight
                    width: parent.width
                    visible: category && block ? !block.collapsed : true

                    Loader {
                        id: decoratorDelegateLoader
                        anchors.fill: parent
                        sourceComponent: Item {
                            anchors.fill: parent
                            StyleItem {
                                anchors.fill: parent
                                elementType: "itemrow"
                                selected: rowitem.DelegateModel.inSelected
                                hasFocus: rowitem.ListView.isCurrentItem
                            }
                            StyleItem {
                                anchors.fill: parent
                                z: 2
                                elementType: "focusrect"
                                visible: rowitem.ListView.isCurrentItem
                            }
                        }
                    }

                    Loader {
                        id: itemDelegateLoader
                        anchors.fill: parent
                        anchors.leftMargin: root.padding
                        anchors.rightMargin: root.padding

                        sourceComponent: root.delegate
                        property var model: itemModel
                        property bool itemSelected: rowitem.DelegateModel.inSelected
                        property int rowIndex: rowitem.rowIndex
                        property int itemWidth: width
                        property color textColor: rowitem.DelegateModel.inSelected ? palette.highlightedText : palette.text
                    }

                    Loader {
                        id: tooltipLoader
                        active: false
                        property var model: itemModel
                        sourceComponent: root.tooltip

                        Timer {
                            id: tooltipTimer
                            interval: 500
                            onTriggered: {
                                tooltipLoader.active = true
                                tooltipLoader.item.state = "poppedUp"
                                stop()
                                tooltipDestructionTimer.restart()
                            }
                        }
                        Timer {
                            id: tooltipDestructionTimer
                            interval: 3000
                            onTriggered: {
                                tooltipLoader.item.state = "poppedDown"
                                stop()
                            }
                        }
                    }

                    MouseArea {
                        id: itemMouseArea
                        anchors.fill: parent
                        acceptedButtons: Qt.LeftButton | Qt.RightButton
                        hoverEnabled: true
                        cursorShape: mouseDragArea.cursorShape

                        onEntered: {
                            if(!tooltipLoader.sourceComponent)
                                return
                            tooltipLoader.x = mouseX
                            tooltipLoader.y = mouseY
                            tooltipTimer.restart()
                        }
                        onExited: {
                            tooltipTimer.stop()
                            tooltipDestructionTimer.stop()
                            tooltipLoader.active = false
                        }

                        onPressed: {
                            console.debug("item pressed")

                            var cur = listView.currentIndex
                            listView.currentIndex = model.index

                            if(mouse.modifiers & Qt.ControlModifier) {
                                console.debug("ctrl+click")

                                rowitem.DelegateModel.inSelected = !rowitem.DelegateModel.inSelected
                            }
                            else if(mouse.modifiers & Qt.ShiftModifier) {
                                console.debug("shift+click")
                                console.debug("model.index: ", cur)
                                console.debug("current: ", cur)
                                if(listView.currentIndex > cur) {
                                    delegateModel.select(cur, listView.currentIndex+1)
                                }
                                else if(cur >= listView.currentIndex) {
                                    delegateModel.select(listView.currentIndex, cur+1)
                                }
                            }
                            else {
                                if(!rowitem.DelegateModel.inSelected) {
                                    clearSelection()
                                }
                                rowitem.DelegateModel.inSelected = true
                            }

                            mouse.accepted = false
                        }

                        onDoubleClicked: {
                            console.debug("item double clicked")
                            if(root.doubleClickAction !== null)
                                root.doubleClickAction(model)
                        }
                    }
                }

                Loader {
                    id: categoryItemLoader
                    x: -parent.x
                    width: listView.width

                    z: -index
                    visible: drawCategory
                    active: drawCategory

                    sourceComponent: Item {
                        id: categoryItem
                        StyleItem {
                            anchors.fill: parent
                            elementType: "itemrow"
                            selected: categoryDelegateLoader.categorySelected
                            active: root.activeFocus
                        }
                        Loader {
                            id: categoryDelegateLoader
                            y: root.padding
                            active: categoryItemLoader.active
                            sourceComponent: category.delegate

                            anchors.margins: root.padding
                            anchors.left: parent.left
                            anchors.right: parent.right

                            Binding {
                                target: categoryItemLoader
                                property: "height"
                                value: categoryDelegateLoader.height + root.padding*2
                            }

                            Binding on height {
                                when: !drawCategory
                                value: 0
                            }

                            property var model: itemModel
                            property bool categorySelected: rowitem.DelegateModel.inSelected
                            property int rowIndex: rowitem.rowIndex
                            property color textColor: categorySelected ? palette.highlightedText : palette.text
                            property int itemCount: block ? block.count : 0
                        }

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: mouseDragArea.cursorShape

                            onPressed: {
                                console.debug("category pressed")

                                var cur = listView.currentIndex
                                listView.currentIndex = model.index

                                if(mouse.modifiers & Qt.ShiftModifier) {
                                    console.debug("shift+click")
                                    console.debug("model.index: ", cur)
                                    console.debug("current: ", cur)
                                    if(listView.currentIndex > cur) {
                                        delegateModel.select(cur, listView.currentIndex+1)
                                    }
                                    else if(cur >= listView.currentIndex) {
                                        delegateModel.select(listView.currentIndex, cur+1)
                                    }
                                }
                                else if(!(mouse.modifiers & Qt.ControlModifier)) {
                                    clearSelection()
                                }

                                delegateModel.select(rowitem.DelegateModel.itemsIndex,
                                                     rowitem.DelegateModel.itemsIndex +
                                                     block.count)

                                mouse.accepted = false
                            }

                            onDoubleClicked: {
                                console.debug("double-click category")
                                block.collapsed = !block.collapsed
                            }
                        }
                    }
                }
            }

            function select(from, to) {
                items.addGroups(from, to-from, "selected")
            }
            function deselect(from, to) {
                items.removeGroups(from, to-from, "selected")
            }
        }

        SystemPalette {
            id: palette
            colorGroup: enabled ? SystemPalette.Active : SystemPalette.Disabled
        }
        Rectangle {
            id: colorRect
            parent: viewport
            anchors.fill: parent
            color: palette.base
            z: -count
        }

        MouseArea {
            id: mouseDragArea
            anchors.fill: parent
            z: -count
            acceptedButtons: Qt.LeftButton | Qt.RightButton
            propagateComposedEvents: true
            property bool dragging: false
            onDraggingChanged: {
                if(dragging)
                    cursorShape = Qt.DragMoveCursor
                else
                    cursorShape = Qt.ArrowCursor
            }

            property int startDragY
            property int modifiers

            onPressed: {
                modifiers = mouse.modifiers
                console.debug("mouse pressed")
                startDragY = mouse.y

                var cur = listView.currentIndex
                listView.currentIndex = listView.indexAt(mouse.x, mouse.y + listView.contentY)

                if(listView.currentIndex === -1) {
                    listView.currentIndex = cur
                    return
                }
            }

            onPositionChanged: {
                if(+(mouse.y - startDragY) > 5 || +(startDragY - mouse.y) > 5)
                    dragging = true
                if(!dragging)
                    return
                var index
                if(mouse.y <= listView.y && listView.contentY > 0) {
                    //scroll up
                    index = listView.indexAt(listView.width/2, listView.contentY)
                    if(index > 0)
                        --index
                    listView.positionViewAtIndex(index, ListView.Beginning)
                    return
                }
                if(mouse.y >= listView.y + listView.height) {
                    //scroll down
                    index = listView.indexAt(listView.width/2, listView.contentY + listView.height-1)
                    if(index === -1)
                        return
                    if(index < listView.count-1)
                        ++index
                    listView.positionViewAtIndex(index, ListView.End)
                    return
                }
            }

            onCanceled: dragging = false
            onReleased: {
                if(dragging) {
                    console.debug("drag finished")
                    var idx = listView.indexAt(mouse.x, mouse.y + listView.contentY)
                    if(mouse.y < 0) {
                        mouse.accepted = false
                        dragging = false
                        return
                    }
                    modifiers = Qt.NoModifier
                    dragging = false

                    moveSelected(idx === -1 ? listView.count : idx)
                }
                else if(listView.indexAt(mouse.x, mouse.y + listView.contentY) === -1) {
                    clearSelection()
                }

                modifiers = Qt.NoModifier
                dragging = false
            }

            onClicked: {
                if(mouse.button == Qt.RightButton && listView.__contextMenu != null) {
                    console.debug("mouseDragArea right-click")
                    listView.__contextMenu.popup()
                }

                if(dragging || listView.indexAt(mouse.x, mouse.y + listView.contentY) === -1) {
                    return
                }
            }
        }
    }
}
