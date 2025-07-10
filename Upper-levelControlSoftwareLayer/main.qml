import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow
{
    id: root
    visible: true
    width: 1200
    height: 800
    minimumWidth: 1100
    minimumHeight: 600
    title: "Rt-smart"

    flags: Qt.Window | Qt.FramelessWindowHint
    color:  "#00000000" //背景透明

    property int resizeAreaWidth: 8


    property alias globalTitleBar: titleBar

    MouseArea
    {
        id:titleMouseArea
        width: parent.width
        height: 50
        onPressed:
        {
            root.startSystemMove()
        }
    }
    background: Rectangle
    {

        radius: root.visibility === Window.Maximized ? 0 : 8
        gradient: Gradient
        {
            GradientStop
            {
                position: 0
                color: "#5ee7df"
            }
            GradientStop
            {
                position: 1
                color: "#b490ca"
            }
            orientation: Gradient.Horizontal
        }
    }


    ColumnLayout
    {
        anchors.fill: parent
        spacing: 0

        CustomTitleBar
        {
            id: titleBar
            Layout.fillWidth: true
            Layout.preferredHeight: 40
            windowTitle: root.title
            targetWindow: root
            backgroundColor: "transparent"
        }


        StackView
        {
            id: stackView
            Layout.fillWidth: true
            Layout.fillHeight: true
           initialItem: "qrc:/LoginPage.qml"

        }
    }

    Connections
    {
        target: stackView.currentItem
        ignoreUnknownSignals: true

        function onLoginSuccessful()
        {
            stackView.push("qrc:/MainPage.qml")
        }

        function onLogoutRequested()
        {
            console.log("注销成功，返回登录页面")
            stackView.pop()
        }
    }

    // 左边缘调整区域
    MouseArea
    {
        id: leftResize
        width: resizeAreaWidth
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        cursorShape: Qt.SizeHorCursor

        onPressed: {
            root.startSystemResize(Qt.LeftEdge)
        }
    }

    // 右边缘调整区域
    MouseArea
    {
        id: rightResize
        width: resizeAreaWidth
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        cursorShape: Qt.SizeHorCursor

        onPressed: {
            root.startSystemResize(Qt.RightEdge)
        }
    }

    // 上边缘调整区域
    MouseArea
    {
        id: topResize
        height: resizeAreaWidth
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        cursorShape: Qt.SizeVerCursor

        onPressed: {
            root.startSystemResize(Qt.TopEdge)
        }
    }

    // 下边缘调整区域
    MouseArea
    {
        id: bottomResize
        height: resizeAreaWidth
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        cursorShape: Qt.SizeVerCursor

        onPressed: {
            root.startSystemResize(Qt.BottomEdge)
        }
    }

    // 左上角调整区域
    MouseArea
    {
        id: topLeftResize
        width: resizeAreaWidth * 2
        height: resizeAreaWidth * 2
        anchors.left: parent.left
        anchors.top: parent.top
        cursorShape: Qt.SizeFDiagCursor

        onPressed: {
            root.startSystemResize(Qt.TopEdge | Qt.LeftEdge)
        }
    }

    // 右上角调整区域
    MouseArea
    {
        id: topRightResize
        width: resizeAreaWidth * 2
        height: resizeAreaWidth * 2
        anchors.right: parent.right
        anchors.top: parent.top
        cursorShape: Qt.SizeBDiagCursor

        onPressed: {
            root.startSystemResize(Qt.TopEdge | Qt.RightEdge)
        }
    }

    // 左下角调整区域
    MouseArea
    {
        id: bottomLeftResize
        width: resizeAreaWidth * 2
        height: resizeAreaWidth * 2
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        cursorShape: Qt.SizeBDiagCursor

        onPressed: {
            root.startSystemResize(Qt.BottomEdge | Qt.LeftEdge)
        }
    }

    // 右下角调整区域
    MouseArea
    {
        id: bottomRightResize
        width: resizeAreaWidth * 2
        height: resizeAreaWidth * 2
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        cursorShape: Qt.SizeFDiagCursor

        onPressed: {
            root.startSystemResize(Qt.BottomEdge | Qt.RightEdge)
        }
    }
}//主窗口
