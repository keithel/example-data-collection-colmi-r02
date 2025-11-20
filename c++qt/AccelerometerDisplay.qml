import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import R02DataExplorer

Item {
    RingConnector {
        id: ring
        Component.onCompleted: startDeviceDiscovery();
        Component.onDestruction: stopDeviceDiscovery();

        allowAutoreconnect: autoreconnectCheckbox.checked

        onAccelerometerDataReady: (x, y, z) => {
            xLabel.text = "X: " + x;
            yLabel.text = "Y: " + y;
            zLabel.text = "Z: " + z;
            bubble.x = (root.width / 2) + (x/30) - (bubble.width/2)
            bubble.y = (root.height / 2) + (y/30) - (bubble.height/2)
        }

        onStatusUpdate: (message) => {
            statusLabel.text = message
            statusLabel.color = "#AAA"
            console.log("[STATUS]", message)
        }

        onError: (message) => {
            statusLabel.text = "Error: " + message
            statusLabel.color = "#FF5555"
            console.log("[ERROR]", message)
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 10

        Label {
            id: statusLabel
            Layout.alignment: Qt.AlignHCenter// | Qt.AlignTop
            text: "Initializing..."
            color: "#AAA"
            font.pixelSize: 16
            font.bold: true
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
            // clip: true // Keep the bubble inside this

            // Centerpoint
            Rectangle {
                anchors.centerIn: parent
                width: 10
                height: 10
                radius: 5
                color: "#333"
            }

            Rectangle {
                id: bubble
                width: 40
                height: 40
                radius: 20
                color: "#2CDE85"
                border.color: "white"
                border.width: 2
                x: parent.width/2 - width/2
                y: parent.height/2 - height/2
                Behavior on x { NumberAnimation { duration: 50; easing.type: Easing.InOutQuad } }
                Behavior on y { NumberAnimation { duration: 50; easing.type: Easing.InOutQuad } }
            }
        }

        RowLayout {
            Layout.alignment: Qt.AlignHCenter
            spacing: 40

            Label {
                id: xLabel
                text: "X: 0"
                color: "white"
                font.pixelSize: 24
                font.family: "Monospace"
            }
            Label {
                id: yLabel
                text: "Y: 0"
                color: "white"
                font.pixelSize: 24
                font.family: "Monospace"
            }
            Label {
                id: zLabel
                text: "Z: 0"
                color: "white"
                font.pixelSize: 24
                font.family: "Monospace"
            }
        }

        RowLayout {
            Layout.alignment: Qt.AlignHCenter
            CheckBox {
                id: autoreconnectCheckbox
                text: "autoreconnect"
            }

            Button {
                text: "Restart Connection"

                onClicked: {
                    console.log("Restarting connection...")
                    ring.startDeviceDiscovery()
                }
            }
        }
    }
}
