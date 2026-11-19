import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    id: root
    width: 1280
    height: 820
    visible: true
    title: qsTr("StoreVitals")
    color: "#0f1419"

    font.family: "Segoe UI Variable"
    font.pixelSize: 14

    readonly property color accent: "#3dd6c6"
    readonly property color panel: "#171d25"
    readonly property color textPrimary: "#edf2f7"
    readonly property color textMuted: "#9aa7b5"

    // Force readable text with Basic style on dark backgrounds.
    palette.window: "#0f1419"
    palette.windowText: textPrimary
    palette.base: "#1b232d"
    palette.text: textPrimary
    palette.button: "#24303c"
    palette.buttonText: textPrimary
    palette.highlight: accent
    palette.highlightedText: "#0b1218"
    palette.placeholderText: textMuted
    palette.mid: "#2a3644"
    palette.dark: "#121820"
    palette.light: "#24303c"

    header: ToolBar {
        id: topBar
        height: 56
        background: Rectangle { color: "#121820" }

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 20
            anchors.rightMargin: 20
            anchors.bottomMargin: 3

            Label {
                text: "StoreVitals"
                color: root.accent
                font.pixelSize: 22
                font.weight: Font.DemiBold
            }

            Item { Layout.fillWidth: true }

            Label {
                text: app.auth.signedIn ? qsTr("Signed in") : qsTr("Not signed in")
                color: app.auth.signedIn ? root.accent : root.textMuted
            }
        }

        // Full-width sync progress — advances per known pipeline stage.
        Rectangle {
            id: syncTrack
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            height: 3
            color: "#1a232d"
            opacity: (app.syncing || syncFill.width > 0) ? 1 : 0
            Behavior on opacity { NumberAnimation { duration: 180 } }

            Rectangle {
                id: syncFill
                anchors.left: parent.left
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                width: parent.width * app.syncProgress
                color: root.accent
                Behavior on width {
                    NumberAnimation {
                        duration: 320
                        easing.type: Easing.OutCubic
                    }
                }
            }
        }
    }

    RowLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            Layout.preferredWidth: 220
            Layout.fillHeight: true
            color: root.panel

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 16
                spacing: 8

                Label {
                    text: qsTr("Navigate")
                    color: root.textMuted
                    font.pixelSize: 12
                }

                Button {
                    Layout.fillWidth: true
                    text: qsTr("Dashboard")
                    flat: true
                    onClicked: stack.replace(dashboardPage)
                    contentItem: Text {
                        text: parent.text
                        color: root.textPrimary
                        horizontalAlignment: Text.AlignLeft
                        verticalAlignment: Text.AlignVCenter
                        font: parent.font
                    }
                    background: Rectangle {
                        radius: 6
                        color: stack.currentItem === dashboardPage ? "#24303c" : "transparent"
                    }
                }

                Button {
                    Layout.fillWidth: true
                    text: qsTr("Settings / Sign in")
                    flat: true
                    onClicked: stack.replace(settingsPage)
                    contentItem: Text {
                        text: parent.text
                        color: root.textPrimary
                        horizontalAlignment: Text.AlignLeft
                        verticalAlignment: Text.AlignVCenter
                        font: parent.font
                    }
                    background: Rectangle {
                        radius: 6
                        color: stack.currentItem === settingsPage ? "#24303c" : "transparent"
                    }
                }

                Item { Layout.fillHeight: true }
            }
        }

        StackView {
            id: stack
            Layout.fillWidth: true
            Layout.fillHeight: true
            initialItem: dashboardPage
        }
    }

    Component {
        id: settingsPage
        SettingsPage { }
    }

    Component {
        id: dashboardPage
        DashboardPage { }
    }

    // Store license gate — bypassed in unpackaged builds via -DDEV_BYPASS_STORE_LICENSE=ON.
    Rectangle {
        anchors.fill: parent
        z: 100
        visible: app.license.ready && !app.license.active
        color: "#e00f1419"
        ColumnLayout {
            anchors.centerIn: parent
            spacing: 12
            width: Math.min(420, parent.width - 48)
            Label {
                text: qsTr("StoreVitals")
                color: root.accent
                font.pixelSize: 28
                font.weight: Font.DemiBold
                Layout.alignment: Qt.AlignHCenter
            }
            Label {
                text: app.license.statusMessage
                color: root.textPrimary
                wrapMode: Text.WordWrap
                horizontalAlignment: Text.AlignHCenter
                Layout.fillWidth: true
            }
            Label {
                text: qsTr("Purchase or redeem StoreVitals from the Microsoft Store to continue. Offline licenses refresh automatically when you reconnect.")
                color: root.textMuted
                wrapMode: Text.WordWrap
                horizontalAlignment: Text.AlignHCenter
                Layout.fillWidth: true
                font.pixelSize: 12
            }
            Button {
                Layout.alignment: Qt.AlignHCenter
                text: qsTr("Retry license check")
                onClicked: app.license.refresh()
                contentItem: Text {
                    text: parent.text
                    color: "#0b1218"
                    font.weight: Font.DemiBold
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle { color: root.accent; radius: 2 }
            }
        }
    }
}
