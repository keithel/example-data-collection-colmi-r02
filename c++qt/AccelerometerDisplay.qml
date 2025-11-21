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
        mouseControlEnabled: mouseControlCheckbox.checked

        onAccelerometerDataReady: (value) => {
            xLabel.text = "X: " + Math.round(value.x);
            yLabel.text = "Y: " + Math.round(value.y);
            zLabel.text = "Z: " + Math.round(value.z);

            // Visual scaling for the bubble so it doesn't fly off screen immediately
            var visualScale = 0.1
            var visualPos = Qt.vector3d(value.x * visualScale, value.y * visualScale, 0)
            bubble.setPos(visualPos);
        }

        onStatusUpdate: (message) => {
            statusLabel.text = message
            statusLabel.color = "#AAA"
            console.log("[STATUS]", message)
        }

        onBatteryLevelReceived: (level, voltage) => {
            batteryLabel.text = "Bat: " + level + "% (" + voltage + "mV)"
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
            id: bubbleParent
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
                color: ring.mouseControlEnabled ? "#FF2CDE" : "#2CDE85"
                border.color: "white"
                border.width: 2

                // Initial pos
                x: parent.width / 2 - width / 2
                y: parent.height / 2 - height / 2

                function setPos(val) {
                    var centerX = parent.width / 2 - width / 2
                    var centerY = parent.height / 2 - height / 2

                    var newX = centerX + val.x
                    var newY = centerY + val.y

                    // Clamp to view
                    x = Math.max(0, Math.min(parent.width - width, newX))
                    y = Math.max(0, Math.min(parent.height - height, newY))
                }

                function reset() {
                    x = parent.width / 2 - width / 2
                    y = parent.height / 2 - height / 2
                }

                Behavior on x { NumberAnimation { duration: 50; easing.type: Easing.OutQuad } }
                Behavior on y { NumberAnimation { duration: 50; easing.type: Easing.OutQuad } }
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

        // Battery Info
        Label {
            id: batteryLabel
            text: "Bat: --%"
            color: "#FFFF00"
            font.pixelSize: 18
            Layout.alignment: Qt.AlignHCenter
        }

        RowLayout {
            Layout.alignment: Qt.AlignHCenter
            CheckBox {
                id: autoreconnectCheckbox
                text: "autoreconnect"
                checked: true
            }

            CheckBox {
                id: mouseControlCheckbox
                text: "mouse control"
            }

            Button {
                text: "Calibrate (Tare)"
                onClicked: {
                    ring.calibrate()
                    bubble.reset()
                }
            }

            Button {
                text: "Restart Connection"

                onClicked: {
                    console.log("Restarting connection...")
                    ring.startDeviceDiscovery()
                }
            }

            Button {
                text: "Get Battery"
                onClicked: ring.getBatteryLevel()
            }

            Button {
                text: "Settings"
                onClicked: settingsDrawer.open()
            }
        }
    }

    // --- Tuning Drawer ---
    Drawer {
        id: settingsDrawer
        edge: Qt.BottomEdge
        width: parent.width
        height: 350
        background: Rectangle { color: "#222"; border.color: "#444" }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 20
            spacing: 10

            Label { text: "Tuning Settings"; color: "white"; font.bold: true; font.pixelSize: 16 }

            // Rotation
            Label { text: `Rotation Correction: ${ring.rotation.toFixed(0)}°`; color: "white" }
            Slider {
                Layout.fillWidth: true
                from: 0; to: 360
                value: ring.rotation
                onMoved: ring.rotation = value
            }

            // Sensitivity
            Label { text: `Sensitivity: ${ring.sensitivity.toFixed(3)}`; color: "white" }
            Slider {
                Layout.fillWidth: true
                from: 0.001; to: 0.1
                value: ring.sensitivity
                onMoved: ring.sensitivity = value
            }

            // Deadzone
            Label { text: `Deadzone: ${ring.deadzone}`; color: "white" }
            Slider {
                Layout.fillWidth: true
                from: 0; to: 500
                value: ring.deadzone
                onMoved: ring.deadzone = value
            }

            // Smoothing
            Label { text: `Smoothing: ${ring.smoothing.toFixed(2)}`; color: "white" }
            Slider {
                Layout.fillWidth: true
                from: 0.0; to: 0.95
                value: ring.smoothing
                onMoved: ring.smoothing = value
            }
        }
    }
}
