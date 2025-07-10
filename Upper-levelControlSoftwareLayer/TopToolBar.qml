import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Controls.Material

Rectangle {
    id: toolbarRoot
    height:60
    property int toolbarHeight: 60
    property color textColor: "white"
    property int currentPageIndex: 0
    signal pageChanged(int pageIndex)

    property int normalButtonSize: 45
    property int activeButtonSize: 55
    property real imageScale: 0.8

    color: "#e6e6e6"
    z: 5  // 提高整个工具栏的 z 值

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 16
        anchors.rightMargin: 16
        spacing: 10
        z: 10


        Rectangle {
            Layout.preferredWidth: toolbarRoot.currentPageIndex === 0 ?
                                  toolbarRoot.activeButtonSize : toolbarRoot.normalButtonSize
            Layout.preferredHeight: toolbarRoot.currentPageIndex === 0 ?
                                   toolbarRoot.activeButtonSize : toolbarRoot.normalButtonSize
            color: "transparent"


            Behavior on Layout.preferredWidth {
                NumberAnimation { duration: 200; easing.type: Easing.OutCubic }
            }
            Behavior on Layout.preferredHeight {
                NumberAnimation { duration: 200; easing.type: Easing.OutCubic }
            }

            Image {
                anchors.centerIn: parent
                width: parent.width * toolbarRoot.imageScale
                height: parent.height * toolbarRoot.imageScale
                source: toolbarRoot.currentPageIndex === 0 ?
                       "qrc:/images/set_active.png" : "qrc:/images/set.png"
                fillMode: Image.PreserveAspectFit
                smooth: true
            }

            MouseArea {
                anchors.fill: parent
                onClicked: {
                    console.log("数据页面按钮被点击")
                    toolbarRoot.pageChanged(0)
                }
            }
        }


        Rectangle {
            Layout.preferredWidth: toolbarRoot.currentPageIndex === 1 ?
                                  toolbarRoot.activeButtonSize : toolbarRoot.normalButtonSize
            Layout.preferredHeight: toolbarRoot.currentPageIndex === 1 ?
                                   toolbarRoot.activeButtonSize : toolbarRoot.normalButtonSize
            color: "transparent"


            Behavior on Layout.preferredWidth {
                NumberAnimation { duration: 200; easing.type: Easing.OutCubic }
            }
            Behavior on Layout.preferredHeight {
                NumberAnimation { duration: 200; easing.type: Easing.OutCubic }
            }

            Image {
                anchors.centerIn: parent
                width: parent.width * toolbarRoot.imageScale
                height: parent.height * toolbarRoot.imageScale
                source: toolbarRoot.currentPageIndex === 1 ?
                       "qrc:/images/charts_active.png" : "qrc:/images/charts.png"
                fillMode: Image.PreserveAspectFit
                smooth: true
            }

            MouseArea {
                anchors.fill: parent
                onClicked: {
                    console.log("图表页面按钮被点击")
                    toolbarRoot.pageChanged(1)
                }
            }
        }


        Item {
            Layout.fillWidth: true
        }

        // 帮助按钮
        Rectangle {
            Layout.preferredWidth: toolbarRoot.currentPageIndex === 2 ?
                                  toolbarRoot.activeButtonSize : toolbarRoot.normalButtonSize
            Layout.preferredHeight: toolbarRoot.currentPageIndex === 2 ?
                                   toolbarRoot.activeButtonSize : toolbarRoot.normalButtonSize
            color: "transparent"


            Behavior on Layout.preferredWidth {
                NumberAnimation { duration: 200; easing.type: Easing.OutCubic }
            }
            Behavior on Layout.preferredHeight {
                NumberAnimation { duration: 200; easing.type: Easing.OutCubic }
            }

            Image {
                anchors.centerIn: parent
                width: parent.width * toolbarRoot.imageScale
                height: parent.height * toolbarRoot.imageScale
                source: toolbarRoot.currentPageIndex === 2 ?
                       "qrc:/images/help_active.png" : "qrc:/images/help.png"
                fillMode: Image.PreserveAspectFit
                smooth: true
            }

            MouseArea {
                anchors.fill: parent
                onClicked: {
                    console.log("帮助页面按钮被点击")
                    toolbarRoot.pageChanged(2)
                }
            }
        }


        Rectangle {
            Layout.preferredWidth: toolbarRoot.currentPageIndex === 3 ?
                                  toolbarRoot.activeButtonSize : toolbarRoot.normalButtonSize
            Layout.preferredHeight: toolbarRoot.currentPageIndex === 3 ?
                                   toolbarRoot.activeButtonSize : toolbarRoot.normalButtonSize
            color: "transparent"

            Behavior on Layout.preferredWidth {
                NumberAnimation { duration: 200; easing.type: Easing.OutCubic }
            }
            Behavior on Layout.preferredHeight {
                NumberAnimation { duration: 200; easing.type: Easing.OutCubic }
            }

            Image {
                anchors.centerIn: parent
                width: parent.width * toolbarRoot.imageScale
                height: parent.height * toolbarRoot.imageScale
                source: toolbarRoot.currentPageIndex === 3 ?
                       "qrc:/images/about_active.png" : "qrc:/images/about.png"
                fillMode: Image.PreserveAspectFit
                smooth: true
            }

            MouseArea {
                anchors.fill: parent
                onClicked: {
                    console.log("关于页面按钮被点击")
                    toolbarRoot.pageChanged(3)
                }
            }
        }
    }

    Rectangle {
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        height: 1
        color: "#ffffff"
        opacity: 0.3
    }
}
