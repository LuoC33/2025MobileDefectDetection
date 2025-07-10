import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Controls.Material
import loging.h 1.0

Page {

    Material.accent: Material.Indigo
    id: loginPageRoot
    signal loginSuccessful()

    Component.onCompleted:
    {
        var mainWindow = Window.window
        if (mainWindow && mainWindow.globalTitleBar)
        {
            mainWindow.globalTitleBar.color = "#00000000"
        }
    }


    background: Rectangle
    {
        color: "transparent"
    }

    LoginManager {
        id: loginManager
        onLoginResult: function(success, message) {
            if (success) {
                errorMessage.visible = false;
                loginSuccessful(); // 触发登录成功信号
            } else {
                errorMessage.text = message;
                errorMessage.visible = true;
            }
        }
    }

    Rectangle {
        width: 800
        height: 500
        radius: 10
        color: "white"
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.verticalCenter: parent.verticalCenter
        border.color: "#cccccc"
        border.width: 1

        RowLayout {
            anchors.fill: parent
            anchors.margins: 50
            spacing: 100

            // 左侧图片
            Image {
                id: image
                source: "qrc:images/img-01.png"
                Layout.alignment: Qt.AlignVCenter
                Layout.preferredWidth: 300
                Layout.preferredHeight: 400
                fillMode: Image.PreserveAspectFit
                states: [
                    State {
                        name: "rotated"
                        PropertyChanges {
                            target: image
                            rotation: 180
                        }
                    }
                ]

                transitions: Transition {
                    RotationAnimation {
                        duration: 1000
                        direction: RotationAnimation.Counterclockwise
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        if(image.state === "rotated") {
                            image.state = ""
                        } else {
                            image.state = "rotated"
                        }
                    }
                }
            }

            ColumnLayout {
                Layout.alignment: Qt.AlignVCenter
                Layout.preferredWidth: 350
                spacing: 20

                Text {
                    text: "用户登录"
                    font.pixelSize: 24
                    font.bold: true
                    Layout.alignment: Qt.AlignHCenter
                }

                TextField {
                    id: usernameField
                    placeholderText: "用户名"
                    Layout.fillWidth: true
                    Layout.preferredHeight: 40
                }

                TextField {
                    id: passwordField
                    placeholderText: "密码"
                    echoMode: TextInput.Password
                    Layout.fillWidth: true
                    Layout.preferredHeight: 40
                }

                Item { Layout.preferredHeight: 20 }

                Button {
                    text: "登录"
                    Layout.fillWidth: true
                    Layout.preferredHeight: 50

                    background: Rectangle {
                        color: parent.down ? "#3057A5" : "#4285F4"
                        radius: 4
                    }

                    contentItem: Text {
                        text: parent.text
                        color: "white"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        font.pixelSize: 16
                    }

                    onClicked: {

                        loginManager.login(usernameField.text, passwordField.text);
                    }
                }

                Text {
                    id: errorMessage
                    text: "用户名或密码错误"
                    color: "red"
                    visible: false
                    Layout.alignment: Qt.AlignHCenter
                }
            }
        }
    }
}
