import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import R02DataExplorer

Item {
    signal showWindowRequested();

    RingConnector {
        id: ring
        Component.onCompleted: startDeviceDiscovery();

        allowAutoreconnect: autoreconnectCheckbox.checked
        mouseControlEnabled: mouseControlCheckbox.checked

        onAccelerometerDataReady: (value) => {
            xLabel.text = "X: " + value.x;
            yLabel.text = "Y: " + value.y;
            zLabel.text = "Z: " + value.z;
            bubble.setPos(value);
        }

        onBatteryLevelChanged: {
            sysTray.toolTip = "Colmi R02: " + batteryLevel + "%"
            sysTray.updateIcon(battIndicator)
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

    Item {
        id: battIndicator
        width: 64
        height: 64
        visible: true

        Rectangle {
            anchors.fill: parent
            color: "transparent"

            Rectangle {
                id: batBody
                x: 10
                y: 20
                width: 44
                height: 24
                color: "transparent"
                border.color: "white"
                border.width: 4

                Rectangle {
                    property int margins: 4
                    anchors {
                        top: parent.top
                        bottom: parent.bottom
                        left: parent.left
                        margins: margins
                    }
                    width: (parent.width - margins*2) * (Math.max(0, ring.batteryLevel) / 100)
                    color: ring.batteryLevel > 20 ? "#4CAF50" : "#F44336"
                }
            }

            Rectangle {
                anchors.left: batBody.right
                anchors.verticalCenter: batBody.verticalCenter
                width: 4
                height: 12
                color: "white"
            }
        }

        SystemTray {
            id: sysTray
            visible: true
            onQuitTriggered: Qt.quit();
            onShowDetailsRequested: showWindowRequested()
            Component.onCompleted: updateIcon(battIndicator)
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
                color: "#2CDE85"
                border.color: "white"
                border.width: 2
                x: parent.width/2 - width/2
                y: parent.height/2 - height/2
                Behavior on x { NumberAnimation { duration: 50; easing.type: Easing.InOutQuad } }
                Behavior on y { NumberAnimation { duration: 50; easing.type: Easing.InOutQuad } }

                function setPos(vector) {
                    var scalefactor = 1
                    var xtmp = (bubbleParent.width / 2) + vector.x/scalefactor - (width/2)
                    if (xtmp + width > bubbleParent.width)
                        x = bubbleParent.width-width
                    else if (xtmp < 0)
                        x = 0
                    else
                        x = xtmp

                    var ytmp = (bubbleParent.height / 2) + vector.y/scalefactor - (height/2)
                    if (ytmp + height > bubbleParent.height)
                        y = bubbleParent.height-height
                    else if (ytmp < 0)
                        y = 0
                    else
                        y = ytmp
                }

                function reset() {
                    x = parent.width/2 - width/2
                    y = parent.height/2 - height/2
                }
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
            property int level: ring.batteryLevel
            property int voltage: ring.batteryVoltage
            onVoltageChanged: console.log("Voltage", voltage)
            text: "Bat: " + (level < 0 ? "--" : level) + (voltage <= 0 ? "%" : ("% (" + voltage + "mV)"))//"Bat: --%"
            color: voltage > 0 ? "#2CDE85" : "#FFFF00"
            font.pixelSize: 18
            Layout.alignment: Qt.AlignHCenter
        }

        RowLayout {
            Layout.alignment: Qt.AlignHCenter
            CheckBox {
                id: autoreconnectCheckbox
                text: "autoreconnect"
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
        }
    }
}
