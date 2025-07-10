import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: aboutPageRoot
    color: "transparent"

    ScrollView {
        anchors.fill: parent
        anchors.margins: 20

        ColumnLayout {
            width: aboutPageRoot.width - 40
            spacing: 30

            ColumnLayout {
                Layout.alignment: Qt.AlignHCenter
                spacing: 15

                Image {
                    source: "qrc:/images/set_active.png"
                    Layout.preferredWidth: 80
                    Layout.preferredHeight: 80
                    Layout.alignment: Qt.AlignHCenter
                    fillMode: Image.PreserveAspectFit
                }

                Text {
                    text: "数据展现软件"
                    font.pixelSize: 28
                    font.bold: true
                    color: "#333333"
                    Layout.alignment: Qt.AlignHCenter
                }

                Text {
                    text: "专业的数据可视化解决方案"
                    font.pixelSize: 16
                    color: "#666666"
                    Layout.alignment: Qt.AlignHCenter
                }
            }

            // 软件信息
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 300
                color: "#ffffff"
                border.color: "#e0e0e0"
                border.width: 1
                radius: 8

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 30
                    spacing: 20

                    RowLayout {
                        Layout.fillWidth: true

                        Text {
                            text: "版本："
                            font.pixelSize: 16
                            font.bold: true
                            color: "#333333"
                            Layout.preferredWidth: 100
                        }

                        Text {
                            text: "v1.0.0"
                            font.pixelSize: 16
                            color: "#666666"
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true

                        Text {
                            text: "作者："
                            font.pixelSize: 16
                            font.bold: true
                            color: "#333333"
                            Layout.preferredWidth: 100
                        }

                        Text {
                            text: "EndlessLeap"
                            font.pixelSize: 16
                            color: "#666666"
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true

                        Text {
                            text: "发布日期："
                            font.pixelSize: 16
                            font.bold: true
                            color: "#333333"
                            Layout.preferredWidth: 100
                        }

                        Text {
                            text: "2025年7月7日"
                            font.pixelSize: 16
                            color: "#666666"
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true

                        Text {
                            text: "许可证："
                            font.pixelSize: 16
                            font.bold: true
                            color: "#333333"
                            Layout.preferredWidth: 100
                        }

                        Text {
                            text: "MIT License"
                            font.pixelSize: 16
                            color: "#666666"
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        height: 1
                        color: "#e0e0e0"
                    }

                    Text {
                        text: "软件描述"
                        font.pixelSize: 16
                        font.bold: true
                        color: "#333333"
                    }

                    Text {
                        text: "这是一款专业的数据展现软件，提供强大的数据可视化功能，支持多种图表类型，帮助用户更好地理解和分析数据。软件界面简洁美观，操作简单直观，适合各种数据分析场景。"
                        font.pixelSize: 14
                        color: "#666666"
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }
                }
            }

            // 技术信息
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 150
                color: "#f8f9fa"
                border.color: "#e0e0e0"
                border.width: 1
                radius: 8

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 20
                    spacing: 15

                    Text {
                        text: "技术栈"
                        font.pixelSize: 16
                        font.bold: true
                        color: "#333333"
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 30

                        ColumnLayout {
                            Text {
                                text: "前端框架"
                                font.pixelSize: 12
                                color: "#666666"
                            }
                            Text {
                                text: "Qt Quick"
                                font.pixelSize: 14
                                font.bold: true
                                color: "#333333"
                            }
                        }

                        ColumnLayout {
                            Text {
                                text: "图表库"
                                font.pixelSize: 12
                                color: "#666666"
                            }
                            Text {
                                text: "Qt Charts"
                                font.pixelSize: 14
                                font.bold: true
                                color: "#333333"
                            }
                        }

                        ColumnLayout {
                            Text {
                                text: "开发语言"
                                font.pixelSize: 12
                                color: "#666666"
                            }
                            Text {
                                text: "QML/C++"
                                font.pixelSize: 14
                                font.bold: true
                                color: "#333333"
                            }
                        }
                    }
                }
            }

            // 版权信息
            Text {
                text: "© 2025 数据展现软件. 保留所有权利."
                font.pixelSize: 12
                color: "#999999"
                Layout.alignment: Qt.AlignHCenter
            }
        }
    }
}
