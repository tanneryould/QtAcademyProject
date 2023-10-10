// Copyright 2023 ESRI
//
// All rights reserved under the copyright laws of the United States
// and applicable international laws, treaties, and conventions.
//
// You may freely redistribute and use this sample code, with or
// without modification, provided you include the original copyright
// notice and use restrictions.
//
// See the Sample code usage restrictions document for further information.
//

import QtQuick
import QtQuick.Controls
import Esri.QtAcademyProject

Item {
    property bool isOffline: false

    // Create MapQuickView here, and create its Map etc. in C++ code
    MapView {
        id: view
        anchors.fill: parent
        // set focus to enable keyboard navigation
        focus: true
    }

    // Declare the C++ instance which creates the map etc. and supply the view
    QtAcademyProject {
        id: model
        mapView: view
    }

    // A background rectangle over which to display the buttons
    Rectangle {
        id: background
        anchors.fill: buttonCol
        anchors.margins: -5
        color: "white"
        radius: 5
    }

    // The column to contain the button images
    Column {
        id: buttonCol
        anchors {
            top: parent.top
            right: parent.right
            margins: 15
        }
        spacing: 10

        // Toggle between tracking the user's location with the line graphic
        Image {
            id: locationImg
            source: model.isTracking ? "qrc:/Resources/calcite-icons/gps-on-32-f.svg" : "qrc:/Resources/calcite-icons/gps-off-32.svg"
            MouseArea {
                anchors.fill: parent
                onClicked: {
                    model.isTracking = !model.isTracking;
                }
            }
        }

        Rectangle {
            height: 1
            width: parent.width
            color: "black"
        }

        // Toggle offline mode
        Image {
            id: switchImg
            source: "qrc:/Resources/calcite-icons/online-32.svg"
            MouseArea {
                anchors.fill: parent
                onClicked: {
                    isOffline = !isOffline;
                    model.toggleOffline(isOffline);
                    if (isOffline)
                        switchImg.source = "qrc:/Resources/calcite-icons/offline-32.svg"
                    else
                        switchImg.source = "qrc:/Resources/calcite-icons/online-32.svg"
                }
            }
        }

        Rectangle {
            height: 1
            width: parent.width
            color: "black"
            visible: !isOffline
        }

        // Download offline map area button
        Image {
            id: offlineImage
            source: "qrc:/Resources/calcite-icons/download-32.svg"
            visible: !isOffline
            MouseArea {
                anchors.fill: parent
                onClicked: {
                    confirmationRect.visible = true
                }
            }
        }
    }

    // Yes or No prompt to download offline area
    Rectangle {
        id: confirmationRect
        anchors.centerIn: parent
        visible: false
        height: 100
        width: 150
        color: "white"
        radius: 5

        Column {
            anchors.centerIn: parent
            spacing: 10

            Text {
                id: downloadConfirmation
                anchors.horizontalCenter: parent.horizontalCenter
                horizontalAlignment: Text.AlignHCenter
                text: "Take area offline?"
            }
            Row {
                anchors.horizontalCenter: parent.horizontalCenter
                spacing: 10
                Button {
                    text: "Yes"
                    onClicked: {

                        model.createOfflineAreaFromExtent();
                        progressRect.visible = true;
                        confirmationRect.visible = false;
                    }
                }
                Button {
                    text: "No"
                    onClicked: confirmationRect.visible = false;
                }
            }
        }
    }

    // Show download progress bar and text percent
    Rectangle {
        id: progressRect
        anchors.centerIn: parent
        visible: false
        height: 100
        width: 150
        color: "white"
        radius: 5

        Column {
            anchors.centerIn: parent
            spacing: 10

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                horizontalAlignment: Text.AlignHCenter
                text: "Download Progress"
            }

            Rectangle {
                height: 5
                width: parent.width
                border.color: "black"
                border.width: 1

                Rectangle {
                    anchors {
                        top: parent.top
                        bottom: parent.bottom
                        left: parent.left
                    }
                    width: (model.downloadProgress / 100) * parent.width
                    color: "purple"

                    onWidthChanged: {
                        if (model.downloadProgress === 100) {
                            progressRect.visible = false;
                            completedNoticeRect.visible = true;
                        }
                    }
                }
            }

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                horizontalAlignment: Text.AlignHCenter
                text: model.downloadProgress + "%"
            }
        }
    }

    // Notify user when download is complete
    Rectangle {
        id: completedNoticeRect
        anchors.centerIn: parent
        visible: false
        height: 100
        width: 150
        color: "white"
        radius: 5

        Column {
            anchors.centerIn: parent
            spacing: 10

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                horizontalAlignment: Text.AlignHCenter
                text: "Download complete!"
            }
            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                text: "Okay"
                onClicked: {
                    completedNoticeRect.visible = false;
                }
            }
        }
    }
}
