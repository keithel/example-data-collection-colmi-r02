// AboutPage.qml
//
// A simple 'About' page for the application.

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Flickable {
    id: flickable

    property int autoShowDuration: 0
    property int flickableOrigWidth: 0

    contentHeight: contentLayout.height
    clip: true

    onWidthChanged: if (flickableOrigWidth == 0) flickableOrigWidth = width

    onAutoShowDurationChanged: {
        if (visible)
            pbar.value = autoShowDuration
    }

    ProgressBar {
        id: pbar
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: 3
        visible: value != to
        from: 0
        to: autoShowDuration
        value: 0
        Behavior on value {
            NumberAnimation { duration: autoShowDuration}
        }

        onVisibleChanged: {
            if (value === 0 && visible)
                value = to
        }

        Component.onCompleted: {
            contentItem.children[0].color = "#2CDE85"
            if (visible)
                value = to
        }
    }

    ScrollBar.vertical: ScrollBar {
        id: scrollBar
    }

    ColumnLayout {
        id: contentLayout
        width: flickable.width - (scrollBar.visible? scrollBar.width : 0)
        anchors.fill: parent
        anchors.margins: 20
        spacing: 15

        ColumnLayout {
            Layout.alignment: Qt.AlignHCenter
            Label {
                text: qsTr("Made With")
                font.pixelSize: 20
                font.bold: true
                Layout.alignment: Qt.AlignHCenter
            }

            Image {
                id: logoImg
                property int desiredWidth: (flickable.width < flickableOrigWidth) ? 224 : 224 * (flickable.width / flickableOrigWidth | 0)
                source: "qrc:/qt/qml/R02DataExplorer/images/qt-logo.svg"
                sourceSize: Qt.size(desiredWidth, desiredWidth*(img.height/img.width))
                Layout.alignment: Qt.AlignHCenter
                fillMode: Image.PreserveAspectFit
                Image {
                    id: img
                    source: parent.source
                    width: 0
                    height: 0
                }
            }
        }

        Label {
            text: qsTr("About This Demonstration")
            font.pixelSize: 24
            font.bold: true
            Layout.alignment: Qt.AlignHCenter
        }

        Label {
            text: qsTr("This application is a data explorer for the Colmi R02 smart ring.")
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }

        Label {
            text: qsTr("It retrieves and shows accelerometer values, and will later show more "
                       + "things.")
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }

        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: "#444"
            Layout.topMargin: 10
            Layout.bottomMargin: 10
        }

        Label {
            text: qsTr("Author: Keith Kyzivat")
            font.italic: true
            Layout.alignment: Qt.AlignHCenter
        }

        Label {
            text: qsTr("Builds off of the fine work of Louis Moreau (luisomoreau) and Wesley Ellis (tahnok)")
            font.italic: true
            Layout.alignment: Qt.AlignHCenter
        }
    }
}
