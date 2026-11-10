import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtMultimedia
//引入 C++ 门面类后，才加回 cn.rh 模块的 import 行
import cn.rh
ApplicationWindow {
    id: root
    width: 900
    height: 650
    visible: true
    title: "人脸识别"
    color: "#1e1e2e"

    property bool isActive: false

    //识别终端：摄像头常开，红外（课堂用定时器模拟）触发后才做识别
    // 画面通过 displaySink 从 C++ 推给 VideoOutput，中间绿色框是人脸引导框
    FaceRecognitionManager{
        id:faceRecognitionManager
    }
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 12

        // ===== 上方：摄像头画面显示框 =====
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.preferredHeight: root.height * 0.7
            radius: 12
            color: "#11111b"
            border.color: "#45475a"
            border.width: 2
            clip: true

            VideoOutput {
                id: videoOutput
                anchors.fill: parent
                anchors.margins: 4
                fillMode: VideoOutput.PreserveAspectCrop
              // `transform: Scale { xScale: -1 ... }` 是 水平镜像：
              // 摄像头拍到的画面像照镜子一样反过来，站在闸机前看才自然。
                transform: Scale { xScale: -1; origin.x: videoOutput.width / 2 }  // 画面修正，水平镜像
            }

            // 人脸引导框：固定在画面中央
            Rectangle {
                id: faceGuide
                width: videoOutput.width * 0.4
                height:  width * 4 / 3         // 人脸大致比例
                anchors.centerIn: parent
                color: "transparent"
                border.color: "#00FF00"
                border.width: 2
                radius: 8
            }

            // 摄像头未就绪时的提示
            Text {
                anchors.centerIn: parent
                text: isActive ? "" : "摄像头未启动"
                color: "#6c7086"
                font.pixelSize: 18
                visible: !isActive
            }

            // 左上角状态小圆点
            Rectangle {
                anchors.left: parent.left
                anchors.top: parent.top
                anchors.margins: 12
                width: 10
                height: 10
                radius: 5
                color: isActive ? "#a6e3a1" : "#f38ba8"
            }
            // 接 C++ 门面类：这里先保持"未启动"状态
            Component.onCompleted: {
                //图像展示
                faceRecognitionManager.displaySink=videoOutput.videoSink
                faceRecognitionManager.openCamera()
                isActive=true
                //摄像头从启动起就一直开着(常开预览)，但不是一直再做识别，只有【有人靠近】这个事件来了，才开启帧送给识别线程，一次识别解释后再重置送帧状态
            }
        }

        // ===== 下方：信息区 + 按钮 =====
        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 120
            spacing: 12

            // 左块：时间与天气
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                radius: 12
                color: "#313244"

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 16
                    spacing: 6

                    // 时间
                    Text {
                        id: timeText
                        text: Qt.formatDateTime(new Date(), "hh:mm:ss")
                        color: "#cdd6f4"
                        font.pixelSize: 34
                        font.bold: true
                    }

                    // 日期
                    Text {
                        text: Qt.formatDateTime(new Date(), "yyyy-MM-dd dddd")
                        color: "#a6adc8"
                        font.pixelSize: 15
                    }

                    // 天气（此处为占位，可对接天气 API 或后端数据）
                    Text {
                        id: weatherText
                        text: "☀️ 晴  24°C"
                        color: "#89b4fa"
                        font.pixelSize: 18
                    }
                }
            }

            // 右块：人脸录入按钮
            Rectangle {
                Layout.preferredWidth: 200
                Layout.fillHeight: true
                radius: 12
                color: "#313244"

                Button {
                    id: enrollButton
                    anchors.centerIn: parent
                    width: 160
                    height: 56
                    text: "人脸录入"

                    background: Rectangle {
                        radius: 10
                        color: enrollButton.down ? "#74c7ec" : (enrollButton.hovered ? "#89dceb" : "#89b4fa")
                    }

                    contentItem: Text {
                        text: enrollButton.text
                        color: "#1e1e2e"
                        font.pixelSize: 18
                        font.bold: true
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }

                    onClicked: {
                        // 扩展点：识别终端也能进录入页，
                        // Day4 不实现，先留按钮占位，避免超纲。
                        console.log("触发人脸录入（扩展功能，Day4 未实现）")
                    }
                }
            }
        }
    }

    // 时间刷新定时器（每秒更新）
    Timer {
        interval: 1000  //触发间隔
        running: true   //定时器是否处于运行状态
        repeat: true    //是否重复触发
        onTriggered: timeText.text = Qt.formatDateTime(new Date(), "hh:mm:ss")
    }
}