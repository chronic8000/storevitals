import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root
    anchors.fill: parent
    visible: open || drawer.x < width
    z: 40

    property bool open: false
    property var review: ({})
    property string storeId: ""

    readonly property color accent: "#3dd6c6"
    readonly property color textPrimary: "#edf2f7"
    readonly property color textMuted: "#8b98a8"
    readonly property color fieldBg: "#151b22"
    readonly property color panel: "#12181f"
    readonly property color line: "#243040"

    readonly property var templates: [
        {
            label: qsTr("Thank you"),
            body: qsTr("Thank you for taking the time to leave a review — feedback like yours helps us improve StoreVitals.")
        },
        {
            label: qsTr("Fixed in latest"),
            body: qsTr("Thanks for reporting this. A fix is included in the latest Store release — please update when you can and let us know if anything else crops up.")
        },
        {
            label: qsTr("Request details"),
            body: qsTr("Thanks for the review. Could you share a bit more detail (steps, package version, and device) so we can investigate? You can reply here or email support.")
        },
        {
            label: qsTr("Sorry + tracking"),
            body: qsTr("Sorry you hit this — we're tracking it against our crash telemetry and prioritizing a fix. Appreciate your patience.")
        }
    ]

    function openReview(r, sid) {
        review = r || {}
        storeId = sid || ""
        draftArea.text = ""
        open = true
    }

    function close() { open = false }

    function insertTemplate(body) {
        if (draftArea.text.length > 0 && !draftArea.text.endsWith("\n"))
            draftArea.text += "\n\n"
        draftArea.text += body
        draftArea.forceActiveFocus()
        draftArea.cursorPosition = draftArea.text.length
    }

    function ramLabel(mb) {
        const n = Number(mb || 0)
        if (n <= 0) return "—"
        if (n >= 1024) return (n / 1024).toFixed(n % 1024 === 0 ? 0 : 1) + " GB"
        return Math.round(n) + " MB"
    }

    Rectangle {
        anchors.fill: parent
        color: "#000000"
        opacity: open ? 0.45 : 0
        visible: opacity > 0.01
        Behavior on opacity { NumberAnimation { duration: 180 } }
        MouseArea {
            anchors.fill: parent
            onClicked: root.close()
        }
    }

    Rectangle {
        id: drawer
        width: Math.min(440, parent.width * 0.92)
        height: parent.height
        x: open ? parent.width - width : parent.width
        color: panel
        border.color: line
        Behavior on x { NumberAnimation { duration: 220; easing.type: Easing.OutCubic } }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 16
            spacing: 10

            RowLayout {
                Layout.fillWidth: true
                Label {
                    text: qsTr("Review Inbox")
                    color: textPrimary
                    font.pixelSize: 16
                    font.weight: Font.DemiBold
                    Layout.fillWidth: true
                }
                Button {
                    text: qsTr("Close")
                    onClicked: root.close()
                    contentItem: Text {
                        text: parent.text
                        color: textPrimary
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    background: Rectangle { color: fieldBg; border.color: line }
                }
            }

            ScrollView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                ColumnLayout {
                    width: drawer.width - 32
                    spacing: 8

                    RowLayout {
                        Label {
                            text: "★".repeat(Math.max(0, Math.min(5, Number(review.rating || 0))))
                            color: accent
                        }
                        Label {
                            text: review.title || qsTr("(no title)")
                            color: textPrimary
                            font.weight: Font.DemiBold
                            wrapMode: Text.WordWrap
                            Layout.fillWidth: true
                        }
                    }
                    Label {
                        text: [review.reviewer || "", review.date || "", review.market || ""].filter(function (s) { return s && String(s).length }).join(" · ")
                        color: textMuted
                        font.pixelSize: 11
                        Layout.fillWidth: true
                    }
                    Label {
                        visible: review.isRevised === true
                        text: qsTr("Revised rating")
                        color: accent
                        font.pixelSize: 11
                    }
                    Label {
                        text: review.text || qsTr("(no review text)")
                        color: textPrimary
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                        font.pixelSize: 13
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: telemetryCol.implicitHeight + 20
                        color: fieldBg
                        border.color: line
                        ColumnLayout {
                            id: telemetryCol
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.top: parent.top
                            anchors.margins: 10
                            spacing: 4
                            Label { text: qsTr("Device telemetry"); color: accent; font.pixelSize: 11; font.weight: Font.DemiBold }
                            Label { text: qsTr("Helpful: %1").arg(review.helpfulCount != null ? review.helpfulCount : 0); color: textMuted; font.pixelSize: 12 }
                            Label { text: qsTr("Package: %1").arg(review.packageVersion || "—"); color: textMuted; font.pixelSize: 12 }
                            Label { text: qsTr("OS: %1").arg(review.osVersion || "—"); color: textMuted; font.pixelSize: 12 }
                            Label { text: qsTr("Device: %1").arg(review.deviceModel || review.deviceType || "—"); color: textMuted; font.pixelSize: 12 }
                            Label { text: qsTr("RAM: %1").arg(root.ramLabel(review.deviceRamMb)); color: textMuted; font.pixelSize: 12 }
                            Label {
                                text: qsTr("Storage: %1").arg(
                                    Number(review.deviceStorageGb || 0) > 0
                                        ? (Number(review.deviceStorageGb).toFixed(0) + " GB")
                                        : "—")
                                color: textMuted
                                font.pixelSize: 12
                            }
                            Label { text: qsTr("Screen: %1").arg(review.screen || "—"); color: textMuted; font.pixelSize: 12 }
                            Label {
                                visible: review.productFamily && String(review.productFamily).length
                                text: qsTr("Family: %1").arg(review.productFamily || "")
                                color: textMuted
                                font.pixelSize: 12
                            }
                        }
                    }

                    Label {
                        text: qsTr("Draft reply")
                        color: textPrimary
                        font.weight: Font.DemiBold
                    }
                    Flow {
                        Layout.fillWidth: true
                        spacing: 6
                        Repeater {
                            model: root.templates
                            delegate: Button {
                                required property var modelData
                                text: modelData.label
                                onClicked: root.insertTemplate(modelData.body)
                                contentItem: Text {
                                    text: parent.text
                                    color: textPrimary
                                    font.pixelSize: 11
                                    horizontalAlignment: Text.AlignHCenter
                                    verticalAlignment: Text.AlignVCenter
                                }
                                background: Rectangle { color: fieldBg; border.color: line; radius: 2 }
                            }
                        }
                    }
                    ScrollView {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 140
                        TextArea {
                            id: draftArea
                            wrapMode: TextEdit.Wrap
                            color: textPrimary
                            font.pixelSize: 13
                            placeholderText: qsTr("Write a reply to paste into Partner Center…")
                            background: Rectangle { color: fieldBg; border.color: line }
                        }
                    }
                    Button {
                        Layout.fillWidth: true
                        text: qsTr("Copy & Open Partner Center")
                        enabled: draftArea.text.trim().length > 0
                        onClicked: app.copyReplyAndOpenPartnerCenter(draftArea.text, root.storeId || app.focusStoreId)
                        contentItem: Text {
                            text: parent.text
                            color: parent.enabled ? "#0b1218" : textMuted
                            font.weight: Font.DemiBold
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        background: Rectangle {
                            color: parent.enabled ? accent : fieldBg
                            border.color: line
                            radius: 2
                        }
                    }
                    Label {
                        text: qsTr("Partner Center no longer accepts review replies via API — paste the draft in the browser.")
                        color: textMuted
                        font.pixelSize: 10
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }
                }
            }
        }
    }
}
