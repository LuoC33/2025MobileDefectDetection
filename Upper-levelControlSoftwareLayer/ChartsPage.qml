import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtCharts
import Network 1.0
import Charts 1.0

Rectangle
{
    id: chartsPageRoot
    color: "transparent"
    LineChart
    {
        id: lineChart
        anchors.fill: parent
    }
}
