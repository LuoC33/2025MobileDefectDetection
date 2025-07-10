import QtQuick
import QtMultimedia

Rectangle {
    id: rtspRoot
    color: "#000000"

    property bool videoPlaying: false
    property string currentUrl: ""


    function playVideo(url) {
        currentUrl = url
        mediaPlayer.source = url
        mediaPlayer.play()
        videoPlaying = true
    }


    function stopVideo() {
        mediaPlayer.stop()
        videoPlaying = false
        currentUrl = ""
    }

    MediaPlayer {
        id: mediaPlayer
        videoOutput: videoOutput

        onPlaybackStateChanged: {
            if (playbackState === MediaPlayer.StoppedState) {
                videoPlaying = false
            } else if (playbackState === MediaPlayer.PlayingState) {
                videoPlaying = true
            }
        }

        onErrorOccurred: function(error, errorString) {
            console.log("RTSP播放错误:", errorString)
            videoPlaying = false
            errorText.text = "连接错误: " + errorString
            errorText.visible = true
        }
    }

    VideoOutput {
        id: videoOutput
        anchors.fill: parent
        fillMode: VideoOutput.PreserveAspectFit
    }

    Text {
        id: errorText
        anchors.centerIn: parent
        text: ""
        color: "white"
        font.pixelSize: 16
        visible: false
        wrapMode: Text.WordWrap
        width: parent.width * 0.8
        horizontalAlignment: Text.AlignHCenter
    }


    Rectangle {
        anchors.centerIn: parent
        width: 200
        height: 100
        color: "transparent"
        border.color: "#666666"
        border.width: 2
        radius: 10
        visible: !videoPlaying

        Text {
            anchors.centerIn: parent
            text: "无视频信号\n点击配置设备连接"
            color: "#666666"
            font.pixelSize: 14
            horizontalAlignment: Text.AlignHCenter
        }
    }


    Rectangle {
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.margins: 10
        width: 150
        height: 60
        color: "#80000000"
        radius: 5
        visible: videoPlaying

        Column {
            anchors.centerIn: parent
            spacing: 5

            Text {
                text: "RTSP视频流"
                color: "white"
                font.pixelSize: 12
                font.bold: true
            }

            Text {
                text: "状态: 播放中"
                color: "white"
                font.pixelSize: 10
            }
        }
    }
}
