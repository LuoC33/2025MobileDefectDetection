import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Controls.Material
Rectangle {
    id: helpPageRoot
    color: "transparent"

    ScrollView
    {
        anchors.fill: parent
        anchors.margins: 20

        ColumnLayout
        {
            width: helpPageRoot.width - 40
            spacing: 20

            // 页面标题
            Text
            {
                text: "帮助文档"
                font.pixelSize: 24
                font.bold: true
                color: "#333333"
                Layout.alignment: Qt.AlignHCenter
            }

            // 帮助内容
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 500
                color: "#ffffff"
                border.color: "#e0e0e0"
                border.width: 1
                radius: 8

                ScrollView {
                    anchors.fill: parent
                    anchors.margins: 20

                    ColumnLayout {
                        width: parent.width
                        spacing: 15

                        // 快速开始
                        Text {
                            text: "快速开始"
                            font.pixelSize: 18
                            font.bold: true
                            color: "#333333"
                        }

                        Text {
                            text: "1. 数据页面：查看和管理您的数据\n2. 图表页面：通过可视化图表分析数据\n3. 帮助页面：获取使用指导\n4. 关于页面：查看软件信息"
                            font.pixelSize: 14
                            color: "#666666"
                            wrapMode: Text.WordWrap
                            Layout.fillWidth: true
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            height: 1
                            color: "#e0e0e0"
                        }

                        // 功能说明
                        Text {
                            text: "功能说明"
                            font.pixelSize: 18
                            font.bold: true
                            color: "#333333"
                        }

                        Text {
                            text: "数据展示功能：\n• 实时数据监控\n• 数据状态管理\n• 统计信息展示\n\n图表分析功能：\n• 趋势分析图表\n• 数据分布饼图\n• 多维数据对比\n• 自定义图表配置"
                            font.pixelSize: 14
                            color: "#666666"
                            wrapMode: Text.WordWrap
                            Layout.fillWidth: true
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            height: 1
                            color: "#e0e0e0"
                        }

                        // 常见问题
                        Text {
                            text: "常见问题"
                            font.pixelSize: 18
                            font.bold: true
                            color: "#333333"
                        }

                        Text {
                            text: "Q: 如何导入数据？\nA: 在数据页面点击导入按钮，选择相应的数据文件。\n\nQ: 图表不显示怎么办？\nA: 请检查数据源是否正确配置，确保有足够的数据用于图表展示。\n\nQ: 如何导出图表？\nA: 在图表页面右键点击图表，选择导出选项。"
                            font.pixelSize: 14
                            color: "#666666"
                            wrapMode: Text.WordWrap
                            Layout.fillWidth: true
                        }
                    }
                }
            }

            // 联系支持
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 100
                color: "#f8f9fa"
                border.color: "#e0e0e0"
                border.width: 1
                radius: 8

                RowLayout {
                    anchors.centerIn: parent
                    spacing: 20

                    Text {
                        text: "需要更多帮助？"
                        font.pixelSize: 16
                        color: "#333333"
                    }

                    Button {
                        text: "联系技术支持"
                        Material.background: "#4CAF50"
                        Material.foreground: "white"

                        onClicked: {
                            console.log("联系技术支持")
                        }
                    }
                }
            }
        }
    }
}
