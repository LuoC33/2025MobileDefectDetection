import QtQuick 6.2
import QtCharts 6.2
import QtQuick.Controls 6.2

Item {
    id: chartContainer
    width: 640
    height: 480

    property var dataManager: null

    function addDataPoint(seriesName, x, y) {
        if (!lineChart.seriesMap[seriesName]) {
            createSeriesIfNeeded(seriesName)
        }

        if (lineChart.seriesMap[seriesName]) {
            lineChart.seriesMap[seriesName].append(x, y)
            console.log("添加数据点到", seriesName, ":", x, y)
        }
    }

    function checkAndUpdateSeries() {
        if (!dataManager || !dataManager.lineSeriesData) return

        let currentSeriesNames = Object.keys(lineChart.seriesMap)
        let newSeriesNames = []

        for (let i = 0; i < dataManager.lineSeriesData.length; i++) {
            newSeriesNames.push(dataManager.lineSeriesData[i].name)
        }

        let needRebuild = false

        for (let name of newSeriesNames) {
            if (!lineChart.seriesMap[name]) {
                needRebuild = true
                break
            }
        }

        for (let name of currentSeriesNames) {
            if (!newSeriesNames.includes(name)) {
                needRebuild = true
                break
            }
        }

        if (needRebuild) {
            console.log("检测到序列结构变化，重建图表")
            rebuildChart()
        }
    }

    function createSeriesIfNeeded(seriesName) {
        if (lineChart.seriesMap[seriesName]) return

        let color = dataManager.getColorForLabel(seriesName)
        let series = lineChart.createSeries(ChartView.SeriesTypeLine, seriesName, axisX, axisY)
        series.color = color
        series.width = 3
        series.pointLabelsVisible = false

        lineChart.seriesMap[seriesName] = series
        console.log("创建新序列:", seriesName)
    }

    function rebuildChart() {
        if (!dataManager || !dataManager.lineSeriesData) return

        console.log("重建折线图，序列数量:", dataManager.lineSeriesData.length)

        lineChart.removeAllSeries()
        lineChart.seriesMap = {}

        for (let i = 0; i < dataManager.lineSeriesData.length; i++) {
            let seriesInfo = dataManager.lineSeriesData[i]

            let series = lineChart.createSeries(ChartView.SeriesTypeLine,
                                              seriesInfo.name, axisX, axisY)
            series.color = seriesInfo.color
            series.width = 3
            series.pointLabelsVisible = false

            if (seriesInfo.data && seriesInfo.data.length > 0) {
                for (let j = 0; j < seriesInfo.data.length; j++) {
                    let point = seriesInfo.data[j]
                    series.append(point.x, point.y)
                }
            }

            lineChart.seriesMap[seriesInfo.name] = series
            console.log("重建序列:", seriesInfo.name, "数据点数量:",
                       seriesInfo.data ? seriesInfo.data.length : 0)
        }
    }

    ChartView {
        id: lineChart
        anchors.fill: parent
        anchors.topMargin: 40
        title: "实时数据折线图"
        animationOptions: ChartView.NoAnimation
        legend.visible: true
        backgroundColor: "white"

        property var seriesMap: ({})

        ValueAxis {
            id: axisX
            min: dataManager ? dataManager.xAxisMin : 0
            max: dataManager ? dataManager.xAxisMax : 10
            tickCount: dataManager ? dataManager.xAxisTickCount : 11
            minorTickCount: dataManager ? dataManager.xAxisMinorTickCount : 10
            titleText: dataManager ? dataManager.xAxisTitle : "时间 (秒)"
        }

        ValueAxis {
            id: axisY
            min: dataManager ? dataManager.yAxisMin : 0
            max: dataManager ? dataManager.yAxisMax : 50
            tickCount: dataManager ? dataManager.yAxisTickCount : 6
            titleText: dataManager ? dataManager.yAxisTitle : "累计数量"
        }

        // 鼠标拖拽区域（仅在固定秒模式下启用）
        MouseArea {
            anchors.fill: parent
            enabled: dataManager && dataManager.chartMode === 1

            property real lastX: 0
            property bool isDragging: false

            onPressed: function(mouse){
                lastX = mouse.x
                isDragging = true
            }

            onPositionChanged:function(mouse) {
                if (!isDragging || !dataManager) return

                let deltaX = mouse.x - lastX
                let chartWidth = lineChart.plotArea.width
                let xRange = axisX.max - axisX.min
                let deltaTime = -(deltaX / chartWidth) * xRange

                let currentOffset = dataManager.xAxisMin
                let newOffset = currentOffset + deltaTime
                let maxOffset = Math.max(0, dataManager.maxDataTime - (axisX.max - axisX.min))

                newOffset = Math.max(0, Math.min(maxOffset, newOffset))

                if (dataManager.setViewOffset) {
                    dataManager.setViewOffset(newOffset)
                }

                lastX = mouse.x
            }

            onReleased: {
                isDragging = false
            }
        }

        // 键盘事件处理
        Keys.onLeftPressed: {
            if (dataManager && dataManager.chartMode === 1) {
                dataManager.panLeft()
            }
        }

        Keys.onRightPressed: {
            if (dataManager && dataManager.chartMode === 1) {
                dataManager.panRight()
            }
        }

        focus: true

        Component.onCompleted: {
            if (dataManager) {
                chartContainer.rebuildChart()
            }
        }
    }

    // 将 Connections 移到 Item 级别
    Connections {
        target: dataManager
        function onXAxisChanged() {
            console.log("X轴参数变化，重建图表")
            rebuildChart()
        }

        function onYAxisChanged() {
            console.log("Y轴范围更新:", dataManager.yAxisMin, "-", dataManager.yAxisMax)
        }

        function onDataPointAdded(seriesName, x, y) {
            addDataPoint(seriesName, x, y)
        }

        function onLineSeriesDataChanged() {
            checkAndUpdateSeries()
        }
    }

    // 顶部控制栏
    Rectangle {
        id: topBar
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: 35
        color: "#f0f0f0"
        border.color: "#d0d0d0"
        border.width: 1

        Row {
            anchors.left: parent.left
            anchors.verticalCenter: parent.verticalCenter
            anchors.leftMargin: 10
            spacing: 10

            Button {
                text: dataManager && dataManager.chartMode === 1 ? "固定秒模式" : "自动模式"
                height: 25
                onClicked: {
                    if (dataManager) {
                        let newMode = dataManager.chartMode === 0 ? 1 : 0
                        dataManager.setChartMode(newMode)
                    }
                }
            }

            Button {
                text: "◀"
                height: 25
                width: 30
                enabled: dataManager && dataManager.canPanLeft
                visible: dataManager && dataManager.chartMode === 1
                onClicked: {
                    if (dataManager) dataManager.panLeft()
                }
            }

            Button {
                text: "▶"
                height: 25
                width: 30
                enabled: dataManager && dataManager.canPanRight
                visible: dataManager && dataManager.chartMode === 1
                onClicked: {
                    if (dataManager) dataManager.panRight()
                }
            }

            Button {
                text: "最新"
                height: 25
                visible: dataManager && dataManager.chartMode === 1
                onClicked: {
                    if (dataManager) dataManager.resetView()
                }
            }
        }

        Text {
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            anchors.rightMargin: 10
            text: dataManager && dataManager.chartMode === 1 && dataManager.firstDataTime ?
                  "开始时间: " + dataManager.firstDataTime : ""
            font.pixelSize: 12
            color: "#666666"
            visible: dataManager && dataManager.chartMode === 1 && dataManager.firstDataTime
        }
    }
}
