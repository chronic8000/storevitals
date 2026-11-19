import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

Item {
    id: page
    readonly property color accent: "#3dd6c6"
    readonly property color textPrimary: "#edf2f7"
    readonly property color textMuted: "#8b98a8"
    readonly property color fieldBg: "#151b22"
    readonly property color panel: "#12181f"
    readonly property color line: "#243040"
    readonly property var dash: app.dashboard

    function openReviewInbox(review) {
        reviewDrawer.openReview(review, dash.selectedStoreId)
    }

    function fmt(n) {
        const v = Number(n || 0)
        if (v >= 1000000) return (v / 1000000).toFixed(1) + "M"
        if (v >= 1000) return (v / 1000).toFixed(1) + "k"
        return String(Math.round(v))
    }

    function fmtMinutes(n) {
        const v = Number(n || 0)
        if (v >= 1000000) return (v / 1000000).toFixed(1) + "M min"
        if (v >= 1000) return (v / 1000).toFixed(1) + "k min"
        return Math.round(v) + " min"
    }

    function fmtMoney(n) {
        const v = Number(n || 0)
        const abs = Math.abs(v)
        const sign = v < 0 ? "-" : ""
        if (abs >= 1000000) return sign + "$" + (abs / 1000000).toFixed(2) + "M"
        if (abs >= 1000) return sign + "$" + (abs / 1000).toFixed(2) + "k"
        return sign + "$" + abs.toFixed(2)
    }

    FileDialog {
        id: exportDialog
        fileMode: FileDialog.SaveFile
        nameFilters: ["CSV files (*.csv)"]
        property string kind: "acquisitions"
        onAccepted: {
            const path = selectedFile.toString()
            if (kind === "installs") dash.exportInstallsCsv(path)
            else if (kind === "failures") dash.exportFailuresCsv(path)
            else dash.exportAcquisitionsCsv(path)
        }
    }

    component MetricChip: Rectangle {
        property string label
        property string value
        property string sub
        Layout.fillWidth: true
        Layout.preferredHeight: 92
        radius: 2
        color: panel
        border.color: line
        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 14
            spacing: 2
            Label { text: label; color: textMuted; font.pixelSize: 11; font.letterSpacing: 0.6 }
            Label { text: value; color: textPrimary; font.pixelSize: 30; font.weight: Font.Medium }
            Label { text: sub; color: textMuted; font.pixelSize: 11; Layout.fillWidth: true; elide: Text.ElideRight }
        }
    }

    component RankList: Rectangle {
        property string title
        property var rows
        property bool visibleWhenEmpty: true
        property bool money: false
        property string emptyText: qsTr("No data for this range")
        // Coalesce blank/whitespace labels so numbering matches what you see.
        readonly property var displayRows: {
            const src = rows || []
            const out = []
            for (let i = 0; i < src.length; ++i) {
                const r = src[i] || {}
                const raw = r.name == null ? "" : String(r.name).trim()
                const label = raw.length > 0 && raw !== "(unknown)" ? raw : qsTr("Unknown")
                out.push({ name: label, value: r.value, conversions: r.conversions })
            }
            return out
        }
        readonly property bool hasRows: displayRows && displayRows.length > 0
        visible: visibleWhenEmpty || hasRows
        Layout.fillWidth: true
        Layout.preferredHeight: 220
        radius: 2
        color: panel
        border.color: line
        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 14
            spacing: 8
            Label { text: title; color: textPrimary; font.pixelSize: 13; font.weight: Font.DemiBold }
            Label {
                visible: !hasRows
                Layout.fillWidth: true
                Layout.fillHeight: true
                text: emptyText
                color: textMuted
                font.pixelSize: 12
                wrapMode: Text.WordWrap
                verticalAlignment: Text.AlignVCenter
            }
            ListView {
                visible: hasRows
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                model: displayRows
                spacing: 6
                delegate: Item {
                    required property var modelData
                    required property int index
                    width: ListView.view.width
                    height: 22
                    RowLayout {
                        anchors.fill: parent
                        spacing: 8
                        Label {
                            text: (index + 1).toString().padStart(2, "0")
                            color: textMuted
                            font.pixelSize: 11
                            Layout.preferredWidth: 22
                        }
                        Label {
                            text: modelData.name
                            color: textPrimary
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                            font.pixelSize: 12
                        }
                        Label {
                            text: money ? page.fmtMoney(modelData.value) : page.fmt(modelData.value)
                            color: accent
                            font.pixelSize: 12
                            font.weight: Font.DemiBold
                        }
                    }
                    Rectangle {
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.bottom: parent.bottom
                        height: 1
                        color: "#1a222c"
                        visible: index < (displayRows.length - 1)
                    }
                }
            }
        }
    }

    RowLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            Layout.preferredWidth: 236
            Layout.fillHeight: true
            color: "#0e1318"
            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 18
                spacing: 10
                Label { text: qsTr("Apps"); color: textMuted; font.pixelSize: 11; font.letterSpacing: 1.2 }
                ListView {
                    id: appList
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    model: dash.apps
                    spacing: 4
                    delegate: Item {
                        required property var modelData
                        width: appList.width
                        height: 58
                        Rectangle {
                            anchors.fill: parent
                            color: modelData.storeId === dash.selectedStoreId ? "#182029" : "transparent"
                            border.color: modelData.storeId === dash.selectedStoreId ? accent : "transparent"
                            border.width: modelData.storeId === dash.selectedStoreId ? 1 : 0
                        }
                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 10
                            spacing: 2
                            Label {
                                text: modelData.displayName
                                color: textPrimary
                                font.weight: Font.DemiBold
                                elide: Text.ElideRight
                                Layout.fillWidth: true
                            }
                            Label {
                                text: qsTr("%1 views · %2 acq").arg(page.fmt(modelData.pageViews)).arg(page.fmt(modelData.acquisitions))
                                color: textMuted
                                font.pixelSize: 11
                            }
                        }
                        MouseArea { anchors.fill: parent; onClicked: dash.selectedStoreId = modelData.storeId }
                    }
                }
                TextField {
                    id: newStoreId
                    Layout.fillWidth: true
                    placeholderText: qsTr("Store ID")
                    color: textPrimary
                    placeholderTextColor: "#5d6b7a"
                    background: Rectangle { color: fieldBg; border.color: line }
                }
                TextField {
                    id: newAppName
                    Layout.fillWidth: true
                    placeholderText: qsTr("Name (optional)")
                    color: textPrimary
                    placeholderTextColor: "#5d6b7a"
                    background: Rectangle { color: fieldBg; border.color: line }
                }
                Button {
                    Layout.fillWidth: true
                    text: qsTr("Add app")
                    onClicked: dash.addApp(newStoreId.text, newAppName.text)
                    contentItem: Text { text: parent.text; color: textPrimary; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                    background: Rectangle { color: "#1a232d"; border.color: line }
                }
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 64
                color: "#0e1318"
                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 16
                    spacing: 10
                    ColumnLayout {
                        spacing: 0
                        Label { text: dash.selectedDisplayName; color: textPrimary; font.pixelSize: 20; font.weight: Font.DemiBold }
                        Label { text: dash.status; color: textMuted; font.pixelSize: 11 }
                    }
                    Item { Layout.fillWidth: true }
                    Button {
                        id: rangeButton
                        implicitWidth: 126
                        text: dash.customRange ? qsTr("Custom") : dash.rangeLabel
                        onClicked: rangeMenu.open()
                        contentItem: Text {
                            text: parent.text
                            color: textPrimary
                            leftPadding: 8
                            rightPadding: 8
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        background: Rectangle { color: fieldBg; border.color: line }
                        Menu {
                            id: rangeMenu
                            y: rangeButton.height
                            width: rangeButton.width
                            property var presets: [
                                { label: "24h", days: 1 },
                                { label: "48h", days: 2 },
                                { label: "3d", days: 3 },
                                { label: "4d", days: 4 },
                                { label: "5d", days: 5 },
                                { label: "6d", days: 6 },
                                { label: "1 week", days: 7 },
                                { label: "2 weeks", days: 14 },
                                { label: "30d", days: 30 },
                                { label: "60d", days: 60 },
                                { label: "90d", days: 90 },
                                { label: "180d", days: 180 },
                                { label: "1 year", days: 365 }
                            ]
                            Repeater {
                                model: rangeMenu.presets
                                MenuItem {
                                    required property var modelData
                                    text: modelData.label
                                    onTriggered: {
                                        customRangeRow.visible = false
                                        dash.windowDays = modelData.days
                                    }
                                }
                            }
                            MenuSeparator {}
                            MenuItem {
                                text: qsTr("Custom…")
                                onTriggered: {
                                    customRangeRow.visible = true
                                    customFromField.text = dash.customFrom
                                    customToField.text = dash.customTo
                                }
                            }
                        }
                    }
                    Button {
                        text: app.syncing ? qsTr("Syncing…") : qsTr("Full sync")
                        enabled: !app.syncing && app.hasApiCredentials
                        onClicked: { app.focusStoreId = dash.selectedStoreId; app.syncNow() }
                        contentItem: Text {
                            text: parent.text
                            color: parent.enabled ? "#071015" : textMuted
                            font.weight: Font.DemiBold
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        background: Rectangle {
                            color: parent.enabled ? accent : "#1a232d"
                            // Keep a quiet visual cue while the top bar carries progress.
                            opacity: app.syncing ? 0.55 : 1
                        }
                    }
                    Button {
                        text: qsTr("Export")
                        onClicked: { exportDialog.kind = "acquisitions"; exportDialog.open() }
                        contentItem: Text { text: parent.text; color: textPrimary; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                        background: Rectangle { color: fieldBg; border.color: line }
                    }
                }
            }

            RowLayout {
                id: customRangeRow
                visible: dash.customRange
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.topMargin: 6
                spacing: 8
                Label { text: qsTr("From"); color: textMuted; font.pixelSize: 11 }
                TextField {
                    id: customFromField
                    Layout.preferredWidth: 120
                    text: dash.customFrom
                    placeholderText: "YYYY-MM-DD"
                    color: textPrimary
                    placeholderTextColor: "#5d6b7a"
                    background: Rectangle { color: fieldBg; border.color: line }
                }
                Label { text: qsTr("To"); color: textMuted; font.pixelSize: 11 }
                TextField {
                    id: customToField
                    Layout.preferredWidth: 120
                    text: dash.customTo
                    placeholderText: "YYYY-MM-DD"
                    color: textPrimary
                    placeholderTextColor: "#5d6b7a"
                    background: Rectangle { color: fieldBg; border.color: line }
                }
                Button {
                    text: qsTr("Apply")
                    onClicked: {
                        dash.applyCustomRange(customFromField.text, customToField.text)
                        customRangeRow.visible = true
                    }
                    contentItem: Text { text: parent.text; color: "#071015"; font.weight: Font.DemiBold; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                    background: Rectangle { color: accent }
                }
                Label {
                    text: qsTr("UTC calendar days · Partner Center data is daily (24h/48h = last 1–2 days)")
                    color: textMuted
                    font.pixelSize: 11
                    Layout.fillWidth: true
                    elide: Text.ElideRight
                }
            }

            Label {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.topMargin: 6
                color: textMuted
                font.pixelSize: 11
                text: qsTr("%1 · %2").arg(dash.rangeLabel).arg(app.syncStatus)
            }

            ScrollView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                contentWidth: availableWidth

                ColumnLayout {
                    width: Math.max(page.width - 236, 800)
                    spacing: 14

                    // Money first — gross sales for the active timeline
                    RowLayout {
                        Layout.fillWidth: true
                        Layout.leftMargin: 16
                        Layout.rightMargin: 16
                        Layout.topMargin: 8
                        spacing: 10
                        MetricChip {
                            label: qsTr("GROSS SALES")
                            value: page.fmtMoney(dash.kpis.grossSalesUsd)
                            sub: qsTr("App %1 · IAP %2 · tax %3")
                                    .arg(page.fmtMoney(dash.kpis.appSalesUsd))
                                    .arg(page.fmtMoney(dash.kpis.iapSalesUsd))
                                    .arg(page.fmtMoney(dash.kpis.taxUsd))
                        }
                        MetricChip {
                            label: qsTr("PAGE VIEWS")
                            value: page.fmt(dash.kpis.pageViews)
                            sub: qsTr("%1% installs/views · %2 listing clicks")
                                    .arg(Number(dash.kpis.conversionRate || 0).toFixed(2))
                                    .arg(page.fmt(dash.kpis.listingClicks))
                        }
                        MetricChip { label: qsTr("ACQUISITIONS"); value: page.fmt(dash.kpis.totalAcquisitions || dash.kpis.funnelAcquisition); sub: qsTr("Got the app") }
                        MetricChip { label: qsTr("INSTALLS"); value: page.fmt(dash.kpis.totalInstalls || dash.kpis.funnelInstall); sub: qsTr("Successful installs") }
                        MetricChip { label: qsTr("USAGE"); value: page.fmt(dash.kpis.funnelUsage || dash.kpis.activeUsers); sub: qsTr("Returned / engaged") }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        Layout.leftMargin: 16
                        Layout.rightMargin: 16
                        spacing: 10
                        MetricChip { label: qsTr("TODAY"); value: page.fmt(dash.kpis.todayAcquisitions); sub: qsTr("Acquisitions UTC") }
                        MetricChip {
                            label: qsTr("ENGAGEMENT")
                            value: page.fmtMinutes(dash.kpis.engagementMinutes)
                            sub: qsTr("%1 sessions · %2 new users")
                                    .arg(page.fmt(dash.kpis.sessions))
                                    .arg(page.fmt(dash.kpis.newUsers))
                        }
                        MetricChip {
                            label: qsTr("DEVICES")
                            value: page.fmt(dash.kpis.activeDevices)
                            sub: qsTr("%1 DAU sum in range").arg(page.fmt(dash.kpis.activeUsers))
                        }
                        MetricChip {
                            label: qsTr("RATING")
                            value: Number(dash.kpis.ratingCount || 0) > 0
                                   ? Number(dash.kpis.averageRating || 0).toFixed(2)
                                   : "—"
                            sub: Number(dash.kpis.ratingCount || 0) > 0
                                 ? qsTr("%1 ratings").arg(page.fmt(dash.kpis.ratingCount))
                                 : qsTr("No ratings yet")
                        }
                        MetricChip { label: qsTr("FAILURES"); value: page.fmt(dash.kpis.failureEvents); sub: qsTr("Health events") }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 250
                        Layout.leftMargin: 16
                        Layout.rightMargin: 16
                        color: panel
                        border.color: line
                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 14
                            Label { text: qsTr("Acquisitions over time"); color: textPrimary; font.weight: Font.DemiBold; font.pixelSize: 13 }
                            Canvas {
                                id: chart
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                property var series: dash.dailySeries
                                onSeriesChanged: requestPaint()
                                onWidthChanged: requestPaint()
                                onHeightChanged: requestPaint()
                                onPaint: {
                                    const ctx = getContext("2d")
                                    ctx.reset()
                                    ctx.fillStyle = "#0e1318"
                                    ctx.fillRect(0, 0, width, height)
                                    const data = series || []
                                    if (!data.length) {
                                        ctx.fillStyle = "#8b98a8"
                                        ctx.font = "13px sans-serif"
                                        ctx.fillText("Run Full sync to load Partner Center analytics", 18, height / 2)
                                        return
                                    }
                                    let maxV = 1
                                    for (let i = 0; i < data.length; ++i)
                                        maxV = Math.max(maxV, Number(data[i].quantity || 0))
                                    const padL = 28, padR = 8, padT = 10, padB = 28
                                    const w = width - padL - padR
                                    const h = height - padT - padB
                                    const n = data.length
                                    // Equal slot per calendar day — never skip empty days.
                                    const gap = w / n
                                    const barW = Math.max(2, gap * 0.64)
                                    ctx.strokeStyle = "#243040"
                                    ctx.beginPath()
                                    ctx.moveTo(padL, padT)
                                    ctx.lineTo(padL, padT + h)
                                    ctx.lineTo(padL + w, padT + h)
                                    ctx.stroke()
                                    for (let i = 0; i < n; ++i) {
                                        const v = Number(data[i].quantity || 0)
                                        const x = padL + i * gap + (gap - barW) / 2
                                        if (v <= 0) {
                                            // Keep the day slot visible with a baseline tick.
                                            ctx.fillStyle = "#2a3542"
                                            ctx.fillRect(x, padT + h - 2, barW, 2)
                                            continue
                                        }
                                        const bh = Math.max(2, (v / maxV) * h)
                                        ctx.fillStyle = "#3dd6c6"
                                        ctx.globalAlpha = 0.9
                                        ctx.fillRect(x, padT + h - bh, barW, bh)
                                    }
                                    ctx.globalAlpha = 1
                                    ctx.fillStyle = "#8b98a8"
                                    ctx.font = "10px sans-serif"
                                    if (n <= 14) {
                                        for (let i = 0; i < n; ++i) {
                                            const label = String(data[i].date).slice(5)
                                            const tw = ctx.measureText(label).width
                                            const cx = padL + i * gap + gap / 2 - tw / 2
                                            ctx.fillText(label, Math.max(padL, cx), height - 8)
                                        }
                                    } else {
                                        ctx.fillText(String(data[0].date).slice(5), padL, height - 8)
                                        const last = String(data[n - 1].date).slice(5)
                                        ctx.fillText(last, width - padR - ctx.measureText(last).width, height - 8)
                                    }
                                }
                            }
                        }
                    }

                    GridLayout {
                        Layout.fillWidth: true
                        Layout.leftMargin: 16
                        Layout.rightMargin: 16
                        columns: 3
                        columnSpacing: 10
                        rowSpacing: 10
                        RankList { title: qsTr("Markets"); rows: dash.markets; emptyText: qsTr("No market breakdown in this range") }
                        RankList { title: qsTr("Devices"); rows: dash.devices; emptyText: qsTr("No device breakdown in this range") }
                        RankList { title: qsTr("OS versions"); rows: dash.osVersions; emptyText: qsTr("No OS breakdown in this range") }
                        RankList { title: qsTr("Package versions"); rows: dash.packageVersions; emptyText: qsTr("No package versions in this range") }
                        RankList { title: qsTr("Revenue by product"); rows: dash.revenueProducts; money: true; emptyText: qsTr("No paid sales in this range") }
                        RankList { title: qsTr("Acquisition type"); rows: dash.acquisitionTypes; emptyText: qsTr("No acquisition types in this range") }
                        RankList { title: qsTr("Store client"); rows: dash.storeClients; emptyText: qsTr("No store client data in this range") }
                        RankList { title: qsTr("Channels (clicks)"); rows: dash.channels; emptyText: qsTr("No channel clicks in this range") }
                        RankList {
                            title: qsTr("Referrers")
                            rows: dash.referrers
                            emptyText: qsTr("No referring domains in this range")
                        }
                        RankList {
                            title: qsTr("Campaigns")
                            rows: dash.campaigns
                            emptyText: qsTr("No campaign IDs in this range")
                        }
                        RankList {
                            title: qsTr("Age groups")
                            rows: dash.ageGroups
                            emptyText: qsTr("Partner Center did not return age groups for this app")
                        }
                        RankList {
                            title: qsTr("Gender")
                            rows: dash.genders
                            emptyText: qsTr("Partner Center did not return gender data for this app")
                        }
                        RankList { title: qsTr("Ratings breakdown"); rows: dash.ratingStars; emptyText: qsTr("No star ratings yet") }
                        RankList {
                            title: qsTr("Architecture (failure hits)")
                            rows: dash.architectures
                            emptyText: qsTr("No architecture data from health events")
                        }
                        RankList {
                            title: qsTr("Crash devices (crash events)")
                            rows: dash.failureDevices
                            emptyText: qsTr("Sync failuredetails for device models")
                        }
                        RankList {
                            title: qsTr("Crash CPUs (crash events)")
                            rows: dash.failureCpus
                            emptyText: qsTr("Sync failuredetails for CPU families")
                        }
                        RankList {
                            title: qsTr("Insider vs retail (crash events)")
                            rows: dash.failureInsider
                            emptyText: qsTr("No Insider/flight ring data yet")
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 280
                        Layout.leftMargin: 16
                        Layout.rightMargin: 16
                        color: panel
                        border.color: line
                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 14
                            RowLayout {
                                Label { text: qsTr("Recent reviews"); color: textPrimary; font.weight: Font.DemiBold; font.pixelSize: 13; Layout.fillWidth: true }
                                Button {
                                    text: qsTr("Review Inbox")
                                    enabled: dash.reviews && dash.reviews.length > 0
                                    onClicked: page.openReviewInbox(dash.reviews[0])
                                    contentItem: Text {
                                        text: parent.text
                                        color: parent.enabled ? textPrimary : textMuted
                                        horizontalAlignment: Text.AlignHCenter
                                        verticalAlignment: Text.AlignVCenter
                                        font.pixelSize: 11
                                    }
                                    background: Rectangle { color: fieldBg; border.color: line }
                                }
                                CheckBox {
                                    text: qsTr("Low RAM (≤8 GB)")
                                    checked: dash.lowRamReviewsOnly
                                    onToggled: dash.lowRamReviewsOnly = checked
                                    contentItem: Text {
                                        text: parent.text
                                        color: textMuted
                                        font.pixelSize: 11
                                        leftPadding: parent.indicator.width + 6
                                        verticalAlignment: Text.AlignVCenter
                                    }
                                }
                            }
                            Label {
                                visible: !(dash.reviews && dash.reviews.length > 0)
                                text: qsTr("No reviews in SQLite yet — Full sync pulls the last few years of reviews.")
                                color: textMuted
                                font.pixelSize: 12
                                Layout.fillWidth: true
                            }
                            ListView {
                                visible: dash.reviews && dash.reviews.length > 0
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                clip: true
                                model: dash.reviews
                                spacing: 10
                                delegate: Item {
                                    required property var modelData
                                    width: ListView.view.width
                                    height: reviewCol.implicitHeight + 4
                                    ColumnLayout {
                                        id: reviewCol
                                        width: parent.width
                                        spacing: 2
                                        RowLayout {
                                            Label { text: "★".repeat(Math.max(0, Math.min(5, modelData.rating))); color: accent }
                                            Label { text: modelData.title || qsTr("(no title)"); color: textPrimary; elide: Text.ElideRight; Layout.fillWidth: true }
                                            Label {
                                                visible: modelData.isRevised === true
                                                text: qsTr("Revised")
                                                color: accent
                                                font.pixelSize: 10
                                            }
                                            Label { text: modelData.date; color: textMuted; font.pixelSize: 11 }
                                        }
                                        Label {
                                            text: {
                                                const bits = []
                                                if (modelData.deviceModel) bits.push(modelData.deviceModel)
                                                if (modelData.deviceRamMb > 0) bits.push(Math.round(modelData.deviceRamMb / 1024) + " GB RAM")
                                                if (modelData.packageVersion) bits.push("v" + modelData.packageVersion)
                                                if (modelData.screen) bits.push(modelData.screen)
                                                return bits.join(" · ")
                                            }
                                            visible: text.length > 0
                                            color: accent
                                            font.pixelSize: 11
                                            elide: Text.ElideRight
                                            Layout.fillWidth: true
                                        }
                                        Flow {
                                            Layout.fillWidth: true
                                            spacing: 4
                                            visible: modelData.topicTags && String(modelData.topicTags).length > 0
                                            Repeater {
                                                model: String(modelData.topicTags || "").split(/[;,|]/).filter(function (t) { return t.trim().length > 0 })
                                                delegate: Rectangle {
                                                    required property string modelData
                                                    color: fieldBg
                                                    border.color: line
                                                    radius: 2
                                                    implicitHeight: tagLbl.implicitHeight + 4
                                                    implicitWidth: tagLbl.implicitWidth + 10
                                                    Label {
                                                        id: tagLbl
                                                        anchors.centerIn: parent
                                                        text: parent.modelData.trim()
                                                        color: textMuted
                                                        font.pixelSize: 10
                                                    }
                                                }
                                            }
                                        }
                                        Label {
                                            text: modelData.text
                                            color: textMuted
                                            wrapMode: Text.WordWrap
                                            maximumLineCount: 2
                                            elide: Text.ElideRight
                                            Layout.fillWidth: true
                                            font.pixelSize: 12
                                        }
                                    }
                                    MouseArea {
                                        anchors.fill: parent
                                        cursorShape: Qt.PointingHandCursor
                                        onClicked: page.openReviewInbox(modelData)
                                    }
                                }
                            }
                        }
                    }

                    Rectangle {
                        visible: dash.versionHealth && dash.versionHealth.length > 0
                        Layout.fillWidth: true
                        Layout.preferredHeight: 240
                        Layout.leftMargin: 16
                        Layout.rightMargin: 16
                        color: panel
                        border.color: line
                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 14
                            spacing: 6
                            Label {
                                text: qsTr("Version health")
                                color: textPrimary
                                font.weight: Font.DemiBold
                                font.pixelSize: 13
                            }
                            Label {
                                text: qsTr("Installs vs active users (DAU sum) vs crash events — thin releases with no usage stand out here")
                                color: textMuted
                                font.pixelSize: 11
                                wrapMode: Text.WordWrap
                                Layout.fillWidth: true
                            }
                            RowLayout {
                                Layout.fillWidth: true
                                Label { text: qsTr("Version"); color: textMuted; font.pixelSize: 10; Layout.fillWidth: true }
                                Label { text: qsTr("Installs"); color: textMuted; font.pixelSize: 10; Layout.preferredWidth: 64; horizontalAlignment: Text.AlignRight }
                                Label { text: qsTr("Active"); color: textMuted; font.pixelSize: 10; Layout.preferredWidth: 64; horizontalAlignment: Text.AlignRight }
                                Label { text: qsTr("Crashes"); color: textMuted; font.pixelSize: 10; Layout.preferredWidth: 64; horizontalAlignment: Text.AlignRight }
                            }
                            ListView {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                clip: true
                                model: dash.versionHealth
                                spacing: 4
                                delegate: ColumnLayout {
                                    required property var modelData
                                    width: ListView.view.width
                                    spacing: 1
                                    RowLayout {
                                        Layout.fillWidth: true
                                        Label {
                                            text: modelData.name
                                            color: modelData.flag ? "#ffb454" : textPrimary
                                            font.pixelSize: 12
                                            elide: Text.ElideRight
                                            Layout.fillWidth: true
                                        }
                                        Label {
                                            text: page.fmt(modelData.installs)
                                            color: textPrimary
                                            font.pixelSize: 12
                                            Layout.preferredWidth: 64
                                            horizontalAlignment: Text.AlignRight
                                        }
                                        Label {
                                            text: page.fmt(modelData.activeUsers)
                                            color: (modelData.activeUsers || 0) === 0 ? "#ffb454" : textPrimary
                                            font.pixelSize: 12
                                            Layout.preferredWidth: 64
                                            horizontalAlignment: Text.AlignRight
                                        }
                                        Label {
                                            text: page.fmt(modelData.crashes)
                                            color: (modelData.crashes || 0) > 0 ? accent : textMuted
                                            font.pixelSize: 12
                                            Layout.preferredWidth: 64
                                            horizontalAlignment: Text.AlignRight
                                        }
                                    }
                                    Label {
                                        visible: modelData.flag && String(modelData.flag).length > 0
                                        text: modelData.flag || ""
                                        color: "#ffb454"
                                        font.pixelSize: 10
                                        elide: Text.ElideRight
                                        Layout.fillWidth: true
                                    }
                                }
                            }
                        }
                    }

                    Rectangle {
                        visible: dash.topFailures && dash.topFailures.length > 0
                        Layout.fillWidth: true
                        Layout.preferredHeight: 260
                        Layout.leftMargin: 16
                        Layout.rightMargin: 16
                        color: panel
                        border.color: line
                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 14
                            RowLayout {
                                Label {
                                    text: qsTr("Top failures (failure hits)")
                                    color: textPrimary
                                    font.weight: Font.DemiBold
                                    Layout.fillWidth: true
                                }
                                Button {
                                    text: qsTr("Export failures")
                                    onClicked: { exportDialog.kind = "failures"; exportDialog.open() }
                                    contentItem: Text { text: parent.text; color: textPrimary; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                                    background: Rectangle { color: fieldBg; border.color: line }
                                }
                            }
                            ListView {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                clip: true
                                model: dash.topFailures
                                spacing: 6
                                delegate: ColumnLayout {
                                    required property var modelData
                                    width: ListView.view.width
                                    spacing: 1
                                    RowLayout {
                                        Layout.fillWidth: true
                                        Label { text: modelData.name; color: textPrimary; elide: Text.ElideRight; Layout.fillWidth: true; font.pixelSize: 12 }
                                        Label {
                                            text: modelData.architecture || "—"
                                            color: (modelData.architecture || "").toLowerCase().indexOf("arm") >= 0 ? "#ffb454" : accent
                                            Layout.preferredWidth: 56
                                            font.pixelSize: 12
                                        }
                                        Label { text: page.fmt(modelData.value); color: textPrimary; Layout.preferredWidth: 48; horizontalAlignment: Text.AlignRight; font.pixelSize: 12 }
                                    }
                                    Label {
                                        visible: modelData.hasStack === true && modelData.stackPreview
                                        text: modelData.stackPreview || ""
                                        color: textMuted
                                        font.pixelSize: 10
                                        font.family: "Consolas"
                                        elide: Text.ElideRight
                                        maximumLineCount: 2
                                        wrapMode: Text.WordWrap
                                        Layout.fillWidth: true
                                    }
                                }
                            }
                        }
                    }

                    Rectangle {
                        visible: dash.failureCabs && dash.failureCabs.length > 0
                        Layout.fillWidth: true
                        Layout.preferredHeight: 220
                        Layout.leftMargin: 16
                        Layout.rightMargin: 16
                        Layout.bottomMargin: 20
                        color: panel
                        border.color: line
                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 14
                            Label {
                                text: qsTr("Crash dumps (CAB)")
                                color: textPrimary
                                font.weight: Font.DemiBold
                                font.pixelSize: 13
                            }
                            Label {
                                text: qsTr("On-demand only — dumps expire; saved under AppData/StoreVitals/cabs/")
                                color: textMuted
                                font.pixelSize: 11
                                Layout.fillWidth: true
                            }
                            ListView {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                clip: true
                                model: dash.failureCabs
                                spacing: 6
                                delegate: RowLayout {
                                    required property var modelData
                                    width: ListView.view.width
                                    spacing: 8
                                    ColumnLayout {
                                        Layout.fillWidth: true
                                        spacing: 1
                                        Label {
                                            text: modelData.name || modelData.cabId
                                            color: textPrimary
                                            elide: Text.ElideRight
                                            Layout.fillWidth: true
                                            font.pixelSize: 12
                                        }
                                        Label {
                                            text: {
                                                const bits = []
                                                if (modelData.deviceModel) bits.push(modelData.deviceModel)
                                                if (modelData.architecture) bits.push(modelData.architecture)
                                                if (modelData.hasStack) bits.push(qsTr("stack"))
                                                if (modelData.cabExpiration) bits.push(qsTr("expires %1").arg(String(modelData.cabExpiration).slice(0, 10)))
                                                return bits.join(" · ")
                                            }
                                            color: textMuted
                                            font.pixelSize: 10
                                            elide: Text.ElideRight
                                            Layout.fillWidth: true
                                        }
                                    }
                                    Button {
                                        text: qsTr("Download dump")
                                        enabled: modelData.downloadable === true && !app.syncing
                                        onClicked: app.downloadCab(modelData.cabId)
                                        contentItem: Text {
                                            text: parent.text
                                            color: parent.enabled ? textPrimary : textMuted
                                            horizontalAlignment: Text.AlignHCenter
                                            verticalAlignment: Text.AlignVCenter
                                            font.pixelSize: 11
                                        }
                                        background: Rectangle { color: fieldBg; border.color: line }
                                    }
                                }
                            }
                        }
                    }

                    Label {
                        Layout.fillWidth: true
                        Layout.leftMargin: 16
                        Layout.rightMargin: 16
                        Layout.bottomMargin: 24
                        wrapMode: Text.WordWrap
                        color: textMuted
                        font.pixelSize: 11
                        text: qsTr("GROSS SALES = Partner Center purchasePriceUSDAmount (app + IAP) for the selected range — same idea as Insights gross sales, not net payouts after refunds. Full sync now pulls app + add-on revenue. Still not wired: subscriptions detail, monthly usage, ads, payouts/earnings exports.")
                    }
                }
            }
        }
    }

    ReviewInboxDrawer {
        id: reviewDrawer
        anchors.fill: parent
    }

    Component.onCompleted: dash.refresh()
}
