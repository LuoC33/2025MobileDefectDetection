import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: dataPageRoot
    color: "transparent"


    property int tcpPort: 8800
    property int udpPort: 9999
    property string rtspUrl: "rtsp://192.168.5.19:8554/test"
    property real confidence: 0.8
    property real yoloThreshold: 0.6
    property real bigsum: 0.6
    property real lightadd: 0.3
    property string currentTask: "BRIDGE" // BRIDGE, PCB, STEEL


    property string k230TcpIP: "192.168.5.19"
    property int k230TcpPort: 8080
    property string k230UdpIP: "192.168.5.19"
    property int k230UdpPort: 8081

    StackView {
        id: stackView
        anchors.fill: parent
        initialItem: mainPage
    }


    Component {
        id: mainPage

        Rectangle {
            color: "transparent"

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 20
                spacing: 20
                RowLayout {
                    Layout.fillWidth: true

                    Text {
                        text: "设备控制"
                        font.pixelSize: 24
                        font.bold: true
                        color: "#333333"
                        Layout.fillWidth: true
                    }

                    Button {
                        text: "设置"
                        font.pixelSize: 14
                        onClicked: {
                            stackView.push(settingsPage)
                        }
                    }
                }
                Rectangle {
                    Layout.fillWidth: true
                    height: 120
                    color: "#f5f5f5"
                    border.color: "#e0e0e0"
                    border.width: 1
                    radius: 8

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 15
                        spacing: 10

                        Text {
                            text: "服务器状态"
                            font.pixelSize: 16
                            font.bold: true
                            color: "#333333"
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 20

                            // TCP服务器状态
                            GroupBox {
                                title: "TCP服务器"
                                Layout.fillWidth: true

                                RowLayout {
                                    anchors.fill: parent

                                    Button {
                                        text: (typeof tcpServer !== "undefined" && tcpServer) ?
                                              (tcpServer.isRunning ? "停止" : "启动") : "启动"
                                        onClicked: {
                                            if (typeof tcpServer !== "undefined" && tcpServer) {
                                                if (tcpServer.isRunning) {
                                                    tcpServer.stopServer()
                                                } else {
                                                    tcpServer.port = dataPageRoot.tcpPort
                                                    tcpServer.startServer()
                                                }
                                            }
                                        }
                                    }

                                    Column
                                    {
                                        Layout.fillWidth: true
                                        Text {
                                            text: "端口: " + dataPageRoot.tcpPort
                                            font.pixelSize: 12
                                        }
                                        Text {
                                            text: "IP: " + ((typeof tcpServer !== "undefined" && tcpServer) ? tcpServer.serverAddress : "")
                                            font.pixelSize: 12
                                            color: "#666666"
                                        }
                                    }

                                    Rectangle {
                                        width: 12
                                        height: 12
                                        radius: 6
                                        color: (typeof tcpServer !== "undefined" && tcpServer && tcpServer.isRunning) ? "green" : "red"
                                    }
                                }
                            }
                            GroupBox {
                                title: "UDP服务器"
                                Layout.fillWidth: true

                                RowLayout {
                                    anchors.fill: parent

                                    Button {
                                        text: (typeof udpServer !== "undefined" && udpServer) ?
                                              (udpServer.isRunning ? "停止" : "启动") : "启动"
                                        onClicked: {
                                            if (typeof udpServer !== "undefined" && udpServer) {
                                                if (udpServer.isRunning) {
                                                    udpServer.stopServer()
                                                } else {
                                                    udpServer.port = dataPageRoot.udpPort
                                                    udpServer.startServer()
                                                }
                                            }
                                        }
                                    }

                                    Column {
                                        Layout.fillWidth: true
                                        Text {
                                            text: "端口: " + dataPageRoot.udpPort
                                            font.pixelSize: 12
                                        }
                                        Text {
                                            text: "IP: " + ((typeof udpServer !== "undefined" && udpServer) ? udpServer.serverAddress : "")
                                            font.pixelSize: 12
                                            color: "#666666"
                                        }
                                    }

                                    Rectangle {
                                        width: 12
                                        height: 12
                                        radius: 6
                                        color: (typeof udpServer !== "undefined" && udpServer && udpServer.isRunning) ? "green" : "red"
                                    }
                                }
                            }
                            GroupBox {
                                title: "视频显示"
                                Layout.fillWidth: true

                                RowLayout {
                                    anchors.fill: parent

                                    Button {
                                        text: rtspComponent.videoPlaying ? "停止" : "启动"
                                        onClicked: {
                                            if (rtspComponent.videoPlaying) {
                                                rtspComponent.stopVideo()
                                            } else {
                                                rtspComponent.playVideo(dataPageRoot.rtspUrl)
                                            }
                                        }
                                    }

                                    Text {
                                        text: "显示实时视频"
                                        Layout.fillWidth: true
                                        font.pixelSize: 12
                                        color: "#666666"
                                    }

                                    Rectangle {
                                        width: 12
                                        height: 12
                                        radius: 6
                                        color: rtspComponent.videoPlaying ? "green" : "red"
                                    }
                                }
                            }
                        }
                    }
                }
                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    color: "#000000"
                    border.color: "#e0e0e0"
                    border.width: 1
                    radius: 8

                    RtspPlayer {
                        id: rtspComponent
                        anchors.fill: parent
                        anchors.margins: 2
                    }
                }
            }
        }
    }
    Component {
        id: settingsPage

        Rectangle
        {
            color: "transparent"

            ScrollView
            {
                anchors.fill: parent
                anchors.margins: 20

                ColumnLayout
                {
                    width: parent.width
                    spacing: 20

                    RowLayout
                    {
                        Layout.fillWidth: true

                        Button {
                            text: "← 返回"
                            onClicked: {
                                stackView.pop()
                            }
                        }

                        Text {
                            text: "推理设置"
                            font.pixelSize: 24
                            font.bold: true
                            color: "#333333"
                            Layout.fillWidth: true
                            horizontalAlignment: Text.AlignHCenter
                        }
                    }
                    GroupBox
                    {
                        title: "本机网络配置"
                        Layout.fillWidth: true

                        GridLayout {
                            anchors.fill: parent
                            columns: 2
                            columnSpacing: 20
                            rowSpacing: 10

                            Text { text: "本机TCP端口:" }
                            SpinBox {
                                from: 1000
                                to: 65535
                                value: dataPageRoot.tcpPort
                                onValueChanged: dataPageRoot.tcpPort = value
                            }

                            Text { text: "本机UDP端口:" }
                            SpinBox {
                                from: 1000
                                to: 65535
                                value: dataPageRoot.udpPort
                                onValueChanged: dataPageRoot.udpPort = value
                            }

                            Text { text: "本机TCP IP:" }
                            Text {
                                text: (typeof tcpServer !== "undefined" && tcpServer) ? tcpServer.serverAddress : "未启动"
                                color: "#666666"
                            }

                            Text { text: "本机UDP IP:" }
                            Text {
                                text: (typeof udpServer !== "undefined" && udpServer) ? udpServer.serverAddress : "未启动"
                                color: "#666666"
                            }

                            Text { text: "视频流地址:" }
                            TextField {
                                Layout.fillWidth: true
                                text: dataPageRoot.rtspUrl
                                onTextChanged: dataPageRoot.rtspUrl = text
                                placeholderText: "rtsp://192.168.5.19:8554/test"
                            }
                        }
                    }

                    GroupBox
                    {
                        title: "工业缺陷检测设备连接配置"
                        Layout.fillWidth: true

                        GridLayout {
                            anchors.fill: parent
                            columns: 2
                            columnSpacing: 20
                            rowSpacing: 10

                            Text { text: "检测设备 TCP IP:" }
                            TextField {
                                Layout.fillWidth: true
                                text: dataPageRoot.k230TcpIP
                                onTextChanged: dataPageRoot.k230TcpIP = text
                                placeholderText: "192.168.5.19"
                            }

                            Text { text: "检测设备 TCP端口:" }
                            SpinBox {
                                from: 1000
                                to: 65535
                                value: dataPageRoot.k230TcpPort
                                onValueChanged: dataPageRoot.k230TcpPort = value
                            }

                            Text { text: "检测设备 UDP IP:" }
                            TextField {
                                Layout.fillWidth: true
                                text: dataPageRoot.k230UdpIP
                                onTextChanged: dataPageRoot.k230UdpIP = text
                                placeholderText: "192.168.5.19"
                            }

                            Text { text: "检测设备 UDP端口:" }
                            SpinBox {
                                from: 1000
                                to: 65535
                                value: dataPageRoot.k230UdpPort
                                onValueChanged: dataPageRoot.k230UdpPort = value
                            }

                            Text { text: "连接状态:" }
                            RowLayout {
                                Layout.fillWidth: true

                                Rectangle {
                                    width: 12
                                    height: 12
                                    radius: 6
                                    color: isK230Connected() ? "green" : "red"
                                }

                                Text {
                                    text: isK230Connected() ? "检测设备已连接" : "检测设备未连接"
                                    color: isK230Connected() ? "green" : "red"
                                    font.pixelSize: 12
                                }
                            }
                        }
                    }
                    GroupBox
                    {
                        title: "推理任务配置"
                        Layout.fillWidth: true

                        ColumnLayout {
                            anchors.fill: parent
                            spacing: 15

                            // 置信度设置
                            RowLayout
                            {
                                Layout.fillWidth: true

                                Text {
                                    text: "置信度: " + dataPageRoot.confidence.toFixed(2)
                                    font.pixelSize: 14
                                }

                                Slider {
                                    Layout.fillWidth: true
                                    from: 0.1
                                    to: 1.0
                                    stepSize: 0.01
                                    value: dataPageRoot.confidence
                                    onValueChanged: dataPageRoot.confidence = value
                                }
                            }
                            RowLayout
                            {
                                Layout.fillWidth: true

                                Text
                                {
                                    text: "光照补偿: " + dataPageRoot.lightadd.toFixed(2)
                                    font.pixelSize: 14
                                }

                                Slider
                                {
                                    Layout.fillWidth: true
                                    from: 0.1
                                    to: 1.0
                                    stepSize: 0.01
                                    value: dataPageRoot.lightadd
                                    onValueChanged: dataPageRoot.lightadd = value
                                }
                            }
                            RowLayout
                            {
                                Layout.fillWidth: true

                                Text {
                                    text: "模型阈值: " + dataPageRoot.yoloThreshold.toFixed(2)
                                    font.pixelSize: 14
                                }

                                Slider {
                                    Layout.fillWidth: true
                                    from: 0.1
                                    to: 1.0
                                    stepSize: 0.01
                                    value: dataPageRoot.yoloThreshold
                                    onValueChanged: dataPageRoot.yoloThreshold = value
                                }
                            }
                            RowLayout
                            {
                                Layout.fillWidth: true

                                Text {
                                    text: "精细程度: " + dataPageRoot.bigsum.toFixed(2)
                                    font.pixelSize: 14
                                }

                                Slider {
                                    Layout.fillWidth: true
                                    from: 0.1
                                    to: 1.0
                                    stepSize: 0.01
                                    value: dataPageRoot.bigsum
                                    onValueChanged: dataPageRoot.bigsum = value
                                }
                            }
                            Text {
                                text: "推理任务:"
                                font.pixelSize: 14
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 10

                                RadioButton {
                                    text: "桥梁缺陷检测"
                                    checked: dataPageRoot.currentTask === "BRIDGE"
                                    onCheckedChanged: {
                                        if (checked) dataPageRoot.currentTask = "BRIDGE"
                                    }
                                }

                                RadioButton {
                                    text: "PCB缺陷检测"
                                    checked: dataPageRoot.currentTask === "PCB"
                                    onCheckedChanged: {
                                        if (checked) dataPageRoot.currentTask = "PCB"
                                    }
                                }

                                RadioButton {
                                    text: "钢材焊接检测"
                                    checked: dataPageRoot.currentTask === "STEEL"
                                    onCheckedChanged: {
                                        if (checked) dataPageRoot.currentTask = "STEEL"
                                    }
                                }
                            }
                            Rectangle {
                                Layout.fillWidth: true
                                height: 60
                                color: "#f0f0f0"
                                border.color: "#d0d0d0"
                                border.width: 1
                                radius: 5

                                Column {
                                    anchors.centerIn: parent
                                    spacing: 5

                                    Text {
                                        text: "当前配置数据包:"
                                        font.pixelSize: 12
                                        color: "#666666"
                                        anchors.horizontalCenter: parent.horizontalCenter
                                    }

                                    Text {
                                        text: "CMD:" + dataPageRoot.currentTask + ":" + dataPageRoot.confidence.toFixed(2)
                                        font.pixelSize: 14
                                        font.bold: true
                                        color: "#333333"
                                        anchors.horizontalCenter: parent.horizontalCenter
                                    }
                                }
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                Layout.topMargin: 20

                                Rectangle {
                                    Layout.fillWidth: true
                                    height: 2
                                    color: "#e0e0e0"
                                }
                            }

                            Button {
                                text: "发送配置到检测设备"
                                font.pixelSize: 16
                                font.bold: true
                                Layout.alignment: Qt.AlignHCenter
                                Layout.preferredWidth: 200
                                Layout.preferredHeight: 40
                                enabled: isK230Connected()

                                onClicked: {
                                    sendConfigToK230()
                                }
                            }
                        }
                    }

                    GroupBox {
                        title: "连接状态"
                        Layout.fillWidth: true

                        ColumnLayout {
                            anchors.fill: parent

                            Text {
                                text: "TCP服务器状态: " + ((typeof tcpServer !== "undefined" && tcpServer && tcpServer.isRunning) ? "运行中" : "未运行")
                                color: (typeof tcpServer !== "undefined" && tcpServer && tcpServer.isRunning) ? "green" : "red"
                            }

                            Text {
                                text: "UDP服务器状态: " + ((typeof udpServer !== "undefined" && udpServer && udpServer.isRunning) ? "运行中" : "未运行")
                                color: (typeof udpServer !== "undefined" && udpServer && udpServer.isRunning) ? "green" : "red"
                            }

                            Text {
                                text: "工业缺陷检测设备连接状态: " + (isK230Connected() ? "已连接" : "未连接")
                                color: isK230Connected() ? "green" : "red"
                            }

                            // 显示所有连接的客户端
                            Text {
                                text: "已连接客户端:"
                                font.pixelSize: 12
                                color: "#666666"
                            }

                            ListView {
                                Layout.fillWidth: true
                                height: 60
                                model: getConnectedClientsList()

                                delegate: Text {
                                    text: "• " + modelData
                                    font.pixelSize: 10
                                    color: "#333333"
                                    padding: 2
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // 检查K230是否连接的函数
    function isK230Connected() {
        if (typeof tcpServer !== "undefined" && tcpServer) {
            let clients = getConnectedClientsList()
            return clients.includes(k230TcpIP)
        }
        return false
    }

    // 获取连接的客户端列表
    function getConnectedClientsList() {
        if (typeof tcpServer !== "undefined" && tcpServer && tcpServer.getConnectedClients) {
            return tcpServer.getConnectedClients()
        }
        return []
    }

    // 发送配置到K230的函数
    function sendConfigToK230() {
        if (typeof tcpServer !== "undefined" && tcpServer && tcpServer.isRunning) {
            let configData = "CMD:" + currentTask + ":" + confidence.toFixed(2)

            if (isK230Connected()) {
                // 发送到指定的K230设备
                if (tcpServer.sendDataToClient) {
                    tcpServer.sendDataToClient(k230TcpIP, configData)
                    console.log("发送配置到检测设备 (" + k230TcpIP + "):", configData)
                } else {
                    console.log("TCP服务器没有sendDataToClient方法")
                }
            } else {
                console.log("检测设备设备未连接，无法发送配置")
            }
        } else {
            console.log("TCP服务器未启动，无法发送配置")
        }
    }
}

