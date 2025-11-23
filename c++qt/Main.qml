import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    id: root
    width: 640
    height: 480
    visible: true
    title: "Colmi R02 Data Explorer"
    color: Qt.black

    Timer {
        id: aboutTimer
        interval: 5000
        running: false

        Component.onCompleted: {
            running = true
            aboutPage.autoShowDuration = interval
        }

        onRunningChanged: if (!running) aboutPage.autoShowDuration = 0
        onTriggered: stackLayout.currentIndex += 1
    }

    StackLayout {
        id: stackLayout
        anchors.fill: parent
        currentIndex: 0
        onCurrentIndexChanged: aboutTimer.stop()

        AboutPage {
            MouseArea {
                anchors.fill: parent
                onClicked: stackLayout.currentIndex += 1
            }

            id: aboutPage
        }
        AccelerometerDisplay {
            onShowWindowRequested: root.visible = true
        }
    }
}
