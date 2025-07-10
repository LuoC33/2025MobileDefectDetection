import QtQuick
import QtQuick.Controls

Rectangle
{
    id: titleBar
    property string windowTitle: ""
    property var targetWindow: null

    property color backgroundColor: "transparent"
    color: backgroundColor
    height: 40

    property color globalcolor: "#1e1e1e"


    radius: targetWindow && targetWindow.background ? targetWindow.background.radius : 0


    Rectangle
    {
        anchors.fill: parent
        color: titleBar.backgroundColor
        radius: titleBar.radius
        clip: true


        Rectangle
        {
            width: parent.width
            height: parent.height / 2
            anchors.bottom: parent.bottom
            color: titleBar.backgroundColor
        }
    }
    Text
    {
        text: titleBar.windowTitle
        color: globalcolor
        font.pixelSize: 14
        font.bold: true
        anchors.left: parent.left
        anchors.leftMargin: 10
        anchors.verticalCenter: parent.verticalCenter
    }

    Row {
        anchors.right: parent.right
        anchors.rightMargin: 5
        anchors.verticalCenter: parent.verticalCenter
        spacing: 0
        Rectangle {
            radius:10
            id: minimizeButton
            width: 40
            height: 30
            color: minimizeMouseArea.containsMouse ? "#34495e" : "transparent"

            Text {
                text: "−"
                color: globalcolor
                font.pixelSize: 16
                anchors.centerIn: parent
            }

            MouseArea {
                id: minimizeMouseArea
                anchors.fill: parent
                hoverEnabled: true
                onClicked: {
                    if (targetWindow) {
                        targetWindow.showMinimized()
                    }
                }
            }
        }
        Rectangle {
            radius:10
            id: maximizeButton
            width: 40
            height: 30
            color: maximizeMouseArea.containsMouse ? "#34495e" : "transparent"

            Text {
                text: targetWindow && targetWindow.visibility === Window.Maximized ? "❐" : "□"
                color: globalcolor
                font.pixelSize: 16
                anchors.centerIn: parent
            }

            MouseArea {
                id: maximizeMouseArea
                anchors.fill: parent
                hoverEnabled: true
                onClicked: {
                    if (targetWindow) {
                        if (targetWindow.visibility === Window.Maximized) {
                            targetWindow.showNormal()
                        } else {
                            targetWindow.showMaximized()
                        }
                    }
                }
            }
        }
        Rectangle {
            id: closeButton
            width: 40
            height: 30
            radius:10
            color: closeMouseArea.containsMouse ? "#e74c3c" : "transparent"

            Text {
                text: "×"
                color: globalcolor
                font.pixelSize: 16
                anchors.centerIn: parent
            }

            MouseArea {
                id: closeMouseArea
                anchors.fill: parent
                hoverEnabled: true
                onClicked: {
                    if (targetWindow) {
                        targetWindow.close()
                    }
                }
            }
        }
    }
}
