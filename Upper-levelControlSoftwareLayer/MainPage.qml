import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtCharts
import QtQuick.Controls.Material

Page {
    Material.foreground: Material.Blue
    id: mainPageRoot
    signal logoutRequested()
    property var window: Window.window
    property int currentPageIndex: 0


    Component.onCompleted:
    {
        window = Window.window

        if (window)
        {
            window.width = 1400
            window.height = 900
            window.x = (Screen.width - window.width) / 2
            window.y = (Screen.height - window.height) / 2
        }

        if (window && window.globalTitleBar)
        {
            window.globalTitleBar.backgroundColor = "#e6e6e6"
        }
    }

    background: Rectangle
    {
        color: "#e6e6e6"

        topLeftRadius: 0
        topRightRadius: 0
        bottomLeftRadius: window && window.background ? window.background.radius : 8
        bottomRightRadius: window && window.background ? window.background.radius : 8
    }



    TopToolBar
    {
        id: topToolbar
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        currentPageIndex: mainPageRoot.currentPageIndex

        onPageChanged: function(pageIndex) {
            mainPageRoot.currentPageIndex = pageIndex
            stackView.currentIndex = pageIndex
        }
    }


    StackLayout
    {
        id: stackView
        anchors.top: topToolbar.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        currentIndex: 0


        DataPage
        {
            id: dataPage
        }

        ChartsPage
        {
            id: chartsPage
        }

        HelpPage {
            id: helpPage
        }

        AboutPage
        {
            id: aboutPage
        }
    }
}
