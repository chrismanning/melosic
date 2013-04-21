import QtQml.Models 2.1
import QtQuick 2.1
import QtQuick.Controls 1.0
import QtQuick.Controls.Private 1.0

import Melosic.Playlist 1.0

ScrollView {
    id: root
    property alias frameVisible: root.frameVisible

    property var model

    property alias delegate: delegateModel.itemDelegate
    property alias currentItem: listView.currentItem
    property alias currentIndex: listView.currentIndex
    property alias count: listView.count
    property int spacing: 0
    property alias categoryModel: categoryModel_
    property int itemHeight: 14
    property var removeItems
    property var moveItems

    property DelegateModelGroup selected: selectedGroup

    property alias category: categoryModel_.category

    Keys.onUpPressed: {
        listView.decrementCurrentIndex()
    }
    Keys.onDownPressed: {
        listView.incrementCurrentIndex()
    }

    function clearSelection() {
        console.debug("clearing selection; count: ", count)
        if(count)
            delegateModel.deselect(0, count)
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

        var groups = contiguousSelected().reverse()
        for(var i = 0; i < groups.length; i++) {
            var g = groups[i]
            if(g.length <= 0)
                continue
            g.sort()
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
            g.sort()
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
            if(groups[j][groups[j].length-1]+1 !== selected.get(i).itemsIndex && groups[j].length > 0) {
                groups.push(new Array)
                ++j
            }
            groups[j].push(selected.get(i).itemsIndex)
        }

        return groups
    }

    function modelIndex(index) {
        return delegateModel.modelIndex(index)
    }

    ListView {
        id: listView
        anchors.fill: parent
        interactive: false

        Binding {
            target: categoryModel_.category
            property: "model"
            when: category !== null
            value: categoryModel_
        }

        model: DelegateModel {
            id: delegateModel
            groups: [ DelegateModelGroup { id: selectedGroup; name: "selected" } ]

            model: CategoryProxyModel {
                id: categoryModel_
                Binding on sourceModel {
                    when: root.model !== null
                    value: root.model
                }
            }

            property Component itemDelegate
            delegate: Item {
                id: rowitem
                x: 5
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
                property CategoryProxyModel categoryModel: categoryModel_
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
                                z: 1
                                elementType: "itemrow"
                                selected: rowitem.DelegateModel.inSelected
                                hasFocus: rowitem.ListView.isCurrentItem
                            }
                            Rectangle {
                                anchors.fill: parent
                                anchors.leftMargin: -1
                                anchors.rightMargin: -1
                                z: 2
                                visible: rowitem.ListView.isCurrentItem
                                border.width: 1
                                border.color: palette.mid
                                color: "transparent"
                            }
                        }
                    }

                    Loader {
                        id: itemDelegateLoader
                        anchors.fill: parent
                        sourceComponent: delegateModel.itemDelegate
                        property var model: itemModel
                        property bool itemSelected: rowitem.DelegateModel.inSelected
                        property int rowIndex: rowitem.rowIndex
                        property int itemWidth: width
                        property color textColor: itemSelected ? palette.highlightedText : palette.text
                    }
                }

                Loader {
                    id: categoryItemLoader
                    x: -parent.x
                    width: listView.width

                    z: -1
                    visible: drawCategory
                    active: drawCategory

                    sourceComponent: Item {
                        id: categoryItem
                        anchors.fill: parent
                        StyleItem {
                            anchors.fill: parent
                            elementType: "itemrow"
                            selected: categoryDelegateLoader.categorySelected
                            active: root.activeFocus
                        }
                        Loader {
                            id: categoryDelegateLoader
                            active: categoryItemLoader.active
                            sourceComponent: category.delegate
                            Binding {
                                target: categoryItemLoader
                                property: "height"
                                value: categoryDelegateLoader.height
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
                            onPressed: {
                                if(mouse.button == Qt.RightButton) {
                                    console.debug("right-click category")
                                }

                                if(mouse.modifiers & Qt.ControlModifier) {
                                    console.debug("ctrl+click category")
                                }
                                else if(mouse.modifiers & Qt.ShiftModifier) {
                                    console.debug("shift+click category")
                                    console.debug("model.index: ", model.index)
                                    var cur = listView.currentIndex
                                    console.debug("current: ", cur)
                                    if(model.index > cur) {
                                        delegateModel.select(cur, model.index)
                                    }
                                    else if(cur >= model.index) {
                                        delegateModel.select(model.index, cur)
                                        rowitem.DelegateModel.inSelected = false
                                    }
                                }
                                else if(mouse.button == Qt.LeftButton) {
//                                    clearSelection()
                                }

                                listView.currentIndex = model.index

                                rowitem.DelegateModel.inSelected = !rowitem.DelegateModel.inSelected

                                delegateModel.select(rowitem.DelegateModel.itemsIndex,
                                                     rowitem.DelegateModel.itemsIndex +
                                                     block.count)
                            }
                            onDoubleClicked: {
                                console.debug("double-click category")
                                block.collapsed = !block.collapsed
                            }
                        }
                    }
                }

                onWidthChanged: listView.contentWidth = width
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
            z: -1
        }

        MouseArea {
            id: mouseDragArea
            anchors.fill: parent
            z: -1
            acceptedButtons: Qt.LeftButton | Qt.RightButton
            propagateComposedEvents: true
            property bool dragging: false
            property int startDragY

            onPressed: {
                console.debug("mouse pressed")
                startDragY = mouse.y

                var cur = listView.currentIndex
                listView.currentIndex = listView.indexAt(mouse.x, mouse.y)

                if(listView.currentIndex === -1) {
                    listView.currentIndex = cur
                    return
                }

                var item = delegateModel.items.get(listView.currentIndex)

                if(mouse.modifiers & Qt.ControlModifier) {
                    console.debug("ctrl+click")

                    item.inSelected = !item.inSelected
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
                else if(mouse.button == Qt.LeftButton) {
                    if(!item.inSelected) {
                        clearSelection()
                    }
                    item.inSelected = true
                }
            }

            onPositionChanged: {
                if(+(mouse.y - startDragY) > 5 || +(startDragY - mouse.y) > 5)
                    dragging = true
                if(!dragging)
                    return
                if(mouse.y <= listView.contentY) {
                    //scroll up
                    return
                }
                if(mouse.y >= listView.contentY) {
                    //scroll down
                    return
                }
            }

            onCanceled: dragging = false
            onReleased: {
                if(dragging) {
                    console.debug("drag finished")
                    var idx = listView.indexAt(mouse.x, mouse.y)
                    if(mouse.y < 0) {
                        mouse.accepted = false
                        dragging = false
                        return
                    }

                    moveSelected(idx === -1 ? listView.count : idx)
                }
                else if(listView.indexAt(mouse.x, mouse.y) === -1) {
                    clearSelection()
                }

                dragging = false
            }

            onClicked: {
                if(dragging || listView.indexAt(mouse.x, mouse.y) === -1) {
                    return
                }

                if(mouse.button == Qt.RightButton) {
                    console.debug("right-click")
                }
            }
        }
    }
}