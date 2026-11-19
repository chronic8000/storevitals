import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: page

    readonly property color accent: "#3dd6c6"
    readonly property color textPrimary: "#edf2f7"
    readonly property color textMuted: "#9aa7b5"
    readonly property color fieldBg: "#1b232d"
    readonly property color danger: "#ff7b72"

    ScrollView {
        anchors.fill: parent
        anchors.margins: 28
        clip: true

        ColumnLayout {
            width: page.width - 56
            spacing: 18

            Label {
                text: qsTr("Sign in with Microsoft")
                color: textPrimary
                font.pixelSize: 28
                font.weight: Font.DemiBold
            }

            Label {
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                color: textMuted
                text: qsTr("Use the same Hotmail, Outlook.com, or work account you use for Partner Center. StoreVitals never stores your password — only a refresh token in Windows Credential Manager.")
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: guide.implicitHeight + 24
                radius: 10
                color: "#152029"
                border.color: "#2a3644"
                border.width: 1

                ColumnLayout {
                    id: guide
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.margins: 12
                    spacing: 8

                    Label {
                        text: qsTr("One-time setup: Application (client) ID")
                        color: accent
                        font.weight: Font.DemiBold
                    }
                    Label {
                        Layout.fillWidth: true
                        wrapMode: Text.WordWrap
                        color: textMuted
                        text: qsTr("1) Supported accounts: any org directory + personal Microsoft accounts.\n2) Authentication → Mobile and desktop redirects: http://localhost AND http://localhost:8400 (exact).\n3) Allow public client flows = Yes.\n4) Paste Application (client) ID below. If browser sign-in fails redirect_uri, use device code.")
                    }
                    RowLayout {
                        Button {
                            text: qsTr("Open app registration portal")
                            onClicked: app.auth.openAzurePortalAppRegistration()
                            contentItem: Text {
                                text: parent.text
                                color: textPrimary
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                                font: parent.font
                            }
                            background: Rectangle {
                                radius: 8
                                color: "#24303c"
                                border.color: "#2a3644"
                            }
                        }
                        Button {
                            text: qsTr("Registration docs")
                            flat: true
                            onClicked: app.auth.openRegistrationGuide()
                            contentItem: Text {
                                text: parent.text
                                color: accent
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                                font: parent.font
                            }
                        }
                    }
                }
            }

            Label {
                text: qsTr("Application (client) ID")
                color: textPrimary
            }
            TextField {
                id: clientIdField
                Layout.fillWidth: true
                text: app.auth.interactiveClientId
                placeholderText: qsTr("xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx")
                color: textPrimary
                placeholderTextColor: "#667584"
                selectedTextColor: "#0b1218"
                selectionColor: accent
                background: Rectangle {
                    radius: 8
                    color: fieldBg
                    border.color: clientIdField.activeFocus ? accent : "#2a3644"
                }
                onTextChanged: app.auth.interactiveClientId = text
            }

            Label {
                text: qsTr("Account type")
                color: textPrimary
            }
            ComboBox {
                id: audienceBox
                Layout.preferredWidth: 420
                model: [
                    { label: qsTr("Personal + work (recommended)"), value: "common" },
                    { label: qsTr("Personal Microsoft accounts only"), value: "consumers" },
                    { label: qsTr("Work / school only"), value: "organizations" }
                ]
                textRole: "label"
                valueRole: "value"
                Component.onCompleted: {
                    const values = ["common", "consumers", "organizations"]
                    currentIndex = Math.max(0, values.indexOf(app.auth.audience))
                }
                onActivated: app.auth.audience = currentValue

                contentItem: Text {
                    leftPadding: 12
                    rightPadding: audienceBox.indicator.width + 12
                    text: audienceBox.displayText
                    font: audienceBox.font
                    color: textPrimary
                    verticalAlignment: Text.AlignVCenter
                    elide: Text.ElideRight
                }

                background: Rectangle {
                    radius: 8
                    color: fieldBg
                    border.color: audienceBox.activeFocus ? accent : "#2a3644"
                }

                indicator: Item {
                    x: audienceBox.width - width - 12
                    y: (audienceBox.height - height) / 2
                    width: 12
                    height: 8
                    Canvas {
                        anchors.fill: parent
                        onPaint: {
                            const ctx = getContext("2d")
                            ctx.clearRect(0, 0, width, height)
                            ctx.fillStyle = "#9aa7b5"
                            ctx.beginPath()
                            ctx.moveTo(0, 0)
                            ctx.lineTo(width, 0)
                            ctx.lineTo(width / 2, height)
                            ctx.closePath()
                            ctx.fill()
                        }
                    }
                }

                popup: Popup {
                    y: audienceBox.height
                    width: audienceBox.width
                    implicitHeight: contentItem.implicitHeight
                    padding: 4
                    contentItem: ListView {
                        clip: true
                        implicitHeight: contentHeight
                        model: audienceBox.popup.visible ? audienceBox.delegateModel : null
                        currentIndex: audienceBox.highlightedIndex
                    }
                    background: Rectangle {
                        radius: 8
                        color: "#171d25"
                        border.color: "#2a3644"
                    }
                }

                delegate: ItemDelegate {
                    width: audienceBox.width
                    required property var modelData
                    required property int index
                    highlighted: audienceBox.highlightedIndex === index
                    contentItem: Text {
                        text: modelData.label
                        color: textPrimary
                        font: audienceBox.font
                        elide: Text.ElideRight
                        verticalAlignment: Text.AlignVCenter
                    }
                    background: Rectangle {
                        color: parent.highlighted ? "#24303c" : "transparent"
                        radius: 6
                    }
                }
            }

            RowLayout {
                spacing: 12

                Button {
                    text: qsTr("Sign in with Microsoft")
                    enabled: !app.auth.busy
                    onClicked: app.auth.signInBrowser()
                    contentItem: Text {
                        text: parent.text
                        color: parent.enabled ? "#0b1218" : "#667584"
                        font.weight: Font.DemiBold
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    background: Rectangle {
                        radius: 8
                        color: parent.enabled ? accent : "#3a4a52"
                    }
                }

                Button {
                    text: qsTr("Use device code")
                    enabled: !app.auth.busy
                    onClicked: app.auth.signInDeviceCode()
                    contentItem: Text {
                        text: parent.text
                        color: textPrimary
                        opacity: parent.enabled ? 1.0 : 0.45
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        font: parent.font
                    }
                    background: Rectangle {
                        radius: 8
                        color: "#24303c"
                        border.color: "#2a3644"
                    }
                }

                Button {
                    text: qsTr("Save")
                    enabled: !app.auth.busy
                    onClicked: app.auth.saveSettings()
                    contentItem: Text {
                        text: parent.text
                        color: textPrimary
                        opacity: parent.enabled ? 1.0 : 0.45
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        font: parent.font
                    }
                    background: Rectangle {
                        radius: 8
                        color: "#24303c"
                        border.color: "#2a3644"
                    }
                }

                Button {
                    text: qsTr("Sign out")
                    enabled: app.auth.signedIn && !app.auth.busy
                    onClicked: app.auth.signOut()
                    contentItem: Text {
                        text: parent.text
                        color: textPrimary
                        opacity: parent.enabled ? 1.0 : 0.45
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        font: parent.font
                    }
                    background: Rectangle {
                        radius: 8
                        color: "#24303c"
                        border.color: "#2a3644"
                    }
                }
            }

            Label {
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                color: textPrimary
                text: app.auth.statusMessage
            }

            Label {
                Layout.fillWidth: true
                visible: app.auth.lastError.length > 0
                wrapMode: Text.WordWrap
                color: danger
                text: app.auth.lastError
            }

            Rectangle {
                visible: app.auth.deviceUserCode.length > 0
                Layout.fillWidth: true
                Layout.preferredHeight: deviceCol.implicitHeight + 24
                radius: 10
                color: "#13251f"
                border.color: accent
                border.width: 1

                ColumnLayout {
                    id: deviceCol
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.margins: 12
                    spacing: 6

                    Label {
                        text: qsTr("Device code sign-in")
                        color: accent
                        font.weight: Font.DemiBold
                    }
                    Label {
                        text: qsTr("Code: %1").arg(app.auth.deviceUserCode)
                        color: textPrimary
                        font.pixelSize: 22
                        font.weight: Font.DemiBold
                    }
                    Label {
                        Layout.fillWidth: true
                        wrapMode: Text.WordWrap
                        color: textMuted
                        text: qsTr("Open %1 and enter the code.").arg(app.auth.deviceVerificationUrl)
                    }
                }
            }

            Label {
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                color: textMuted
                text: qsTr("Use Personal + work for Hotmail sign-in. Analytics Sync is separate (section below).")
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: apiCol.implicitHeight + 24
                radius: 10
                color: "#152029"
                border.color: "#2a3644"
                border.width: 1

                ColumnLayout {
                    id: apiCol
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.margins: 12
                    spacing: 10

                    Label {
                        text: qsTr("API credentials (required for Sync)")
                        color: accent
                        font.weight: Font.DemiBold
                    }
                    Label {
                        Layout.fillWidth: true
                        wrapMode: Text.WordWrap
                        color: textMuted
                        text: qsTr("Hotmail Partner Center accounts are fine for publishing. Microsoft’s analytics API still needs a one-time Entra link: Tenants → associate directory → User management → Microsoft Entra applications → add your Entra app (Manager). Then paste tenant ID, client ID, and client secret here. Secret stays in Windows Credential Manager only.")
                    }
                    RowLayout {
                        Button {
                            text: qsTr("1) Open Tenants (do this first)")
                            onClicked: app.auth.openPartnerCenterTenants()
                            contentItem: Text {
                                text: parent.text
                                color: "#0b1218"
                                font.weight: Font.DemiBold
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                            background: Rectangle { radius: 8; color: accent }
                        }
                        Button {
                            text: qsTr("2) User management")
                            onClicked: app.auth.openPartnerCenterUserManagement()
                            contentItem: Text {
                                text: parent.text
                                color: textPrimary
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                            background: Rectangle {
                                radius: 8
                                color: "#24303c"
                                border.color: "#2a3644"
                            }
                        }
                    }

                    Label { text: qsTr("Directory (tenant) ID"); color: textPrimary }
                    TextField {
                        Layout.fillWidth: true
                        text: app.auth.tenantId
                        color: textPrimary
                        placeholderText: qsTr("xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx")
                        placeholderTextColor: "#667584"
                        background: Rectangle {
                            radius: 8
                            color: fieldBg
                            border.color: "#2a3644"
                        }
                        onTextChanged: app.auth.tenantId = text
                    }

                    Label { text: qsTr("Application (client) ID for API"); color: textPrimary }
                    TextField {
                        Layout.fillWidth: true
                        text: app.auth.apiClientId
                        color: textPrimary
                        placeholderText: qsTr("Usually the same Entra app client ID")
                        placeholderTextColor: "#667584"
                        background: Rectangle {
                            radius: 8
                            color: fieldBg
                            border.color: "#2a3644"
                        }
                        onTextChanged: app.auth.apiClientId = text
                    }

                    Label { text: qsTr("Client secret"); color: textPrimary }
                    TextField {
                        id: secretField
                        Layout.fillWidth: true
                        echoMode: TextInput.Password
                        color: textPrimary
                        placeholderText: app.auth.hasApiSecret
                                         ? qsTr("(secret already stored — paste to replace)")
                                         : qsTr("Paste client secret value")
                        placeholderTextColor: "#667584"
                        background: Rectangle {
                            radius: 8
                            color: fieldBg
                            border.color: "#2a3644"
                        }
                    }

                    RowLayout {
                        Button {
                            text: qsTr("Save API settings")
                            onClicked: {
                                app.auth.saveApiSettings()
                                if (secretField.text.length > 0) {
                                    app.auth.storeApiClientSecret(secretField.text)
                                    secretField.text = ""
                                }
                                app.refreshApiCredentialState()
                            }
                            contentItem: Text {
                                text: parent.text
                                color: "#0b1218"
                                font.weight: Font.DemiBold
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                            background: Rectangle {
                                radius: 8
                                color: accent
                            }
                        }
                        Button {
                            text: qsTr("Clear secret")
                            onClicked: {
                                app.auth.clearApiClientSecret()
                                app.refreshApiCredentialState()
                            }
                            contentItem: Text {
                                text: parent.text
                                color: textPrimary
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                            background: Rectangle {
                                radius: 8
                                color: "#24303c"
                                border.color: "#2a3644"
                            }
                        }
                        Label {
                            text: app.hasApiCredentials ? qsTr("API ready") : qsTr("API incomplete")
                            color: app.hasApiCredentials ? accent : "#ffb454"
                        }
                    }
                }
            }
        }
    }
}
