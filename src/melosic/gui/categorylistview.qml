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
    property Component decoratorDelegate: nativeDelegate
    property alias currentItem: listView.currentItem
    property alias currentIndex: listView.currentIndex
    property alias count: listView.count
    property int spacing: 0
    property alias categoryModel: categoryModel_
    property int itemHeight: 14

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

                    MouseArea {
                        anchors.fill: parent
                        acceptedButtons: Qt.LeftButton | Qt.RightButton
                        onClicked: {
                            if(mouse.button == Qt.RightButton) {
                                console.debug("right-click")
                            }

                            if(mouse.modifiers & Qt.ControlModifier) {
                                console.debug("ctrl+click")
                            }
                            else if(mouse.modifiers & Qt.ShiftModifier) {
                                console.debug("shift+click")
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
                                clearSelection()
                            }

                            listView.currentIndex = model.index

                            rowitem.DelegateModel.inSelected = !rowitem.DelegateModel.inSelected
                        }
                    }

                    Loader {
                        id: decoratorDelegateLoader
                        anchors.fill: parent
                        sourceComponent: decoratorDelegate
                        property bool itemSelected: rowitem.DelegateModel.inSelected
                        property int itemWidth: width
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
                            onClicked: {
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
                                    clearSelection()
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
                console.debug("selecting items ", from, " to ", to)
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
            anchors.fill: parent
            onClicked: clearSelection()
            z: -1
        }

        Component {
            id: nativeDelegate
            StyleItem {
                elementType: "itemrow"
                selected: itemSelected
                active: root.activeFocus
            }
        }
    }
}
