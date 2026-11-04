/*
 * SPDX-FileCopyrightText: 2019 Nicolas Fella <nicolas.fella@gmx.de>
 * SPDX-FileCopyrightText: 2026 KDE Connect contributors
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kdeconnect as KDEConnect
import org.kde.kirigami as Kirigami
import org.kde.kitemmodels as KItemModels

Kirigami.ScrollablePage {
    id: page

    required property var pluginInterface
    required property var device

    title: i18nc("@title:window", "Notifications")

    property string searchFilter: ""
    property string activeFilter: "all" // "all", "repliable", "dismissable"

    // Top action bar: global "Clear All" action
    actions: [
        Kirigami.Action {
            text: i18nc("@action:button", "Clear All")
            icon.name: "edit-clear-all"
            visible: notificationsModel.isAnyDismissable
            onTriggered: notificationsModel.dismissAll()
        }
    ]

    // Authoritative D-Bus notifications model for this device
    KDEConnect.NotificationsModel {
        id: notificationsModel
        deviceId: page.device ? page.device.id() : ""
    }

    // Proxy model: sorts by appName for clean grouping and filters by search text or category
    KItemModels.KSortFilterProxyModel {
        id: filteredNotificationsModel
        sourceModel: notificationsModel
        sortRoleName: "appName"
        sortCaseSensitivity: Qt.CaseInsensitive

        filterRoleName: {
            if (page.activeFilter === "repliable") {
                return "repliable";
            } else if (page.activeFilter === "dismissable") {
                return "dismissable";
            }
            return "";
        }

        filterString: {
            if (page.activeFilter === "repliable" || page.activeFilter === "dismissable") {
                return "true";
            }
            return page.searchFilter;
        }

        filterCaseSensitivity: Qt.CaseInsensitive
    }

    // Modern Header: Search bar, filter pills (All / Can Reply / Dismissable), and quick Clear All
    header: QQC2.Control {
        visible: notificationsModel.count > 0
        topPadding: Kirigami.Units.smallSpacing
        bottomPadding: Kirigami.Units.smallSpacing
        leftPadding: Kirigami.Units.largeSpacing
        rightPadding: Kirigami.Units.largeSpacing

        background: Rectangle {
            Kirigami.Theme.colorSet: Kirigami.Theme.Window
            Kirigami.Theme.inherit: false
            color: Kirigami.Theme.backgroundColor

            Kirigami.Separator {
                anchors {
                    left: parent.left
                    bottom: parent.bottom
                    right: parent.right
                }
            }
        }

        contentItem: ColumnLayout {
            spacing: Kirigami.Units.smallSpacing

            // Row 1: Search Box + Clear All button
            RowLayout {
                spacing: Kirigami.Units.mediumSpacing
                Layout.fillWidth: true

                Kirigami.SearchField {
                    id: searchBox
                    Layout.fillWidth: true
                    placeholderText: i18nc("@info:placeholder", "Search notifications...")
                    onTextChanged: {
                        page.searchFilter = text;
                        if (text.length > 0) {
                            page.activeFilter = "all";
                        }
                    }
                    autoAccept: false
                }

                QQC2.Button {
                    text: i18nc("@action:button", "Clear All")
                    icon.name: "edit-clear-all"
                    visible: notificationsModel.isAnyDismissable
                    onClicked: notificationsModel.dismissAll()
                }
            }

            // Row 2: Filter Pills (All / Can Reply / Dismissable)
            RowLayout {
                spacing: Kirigami.Units.smallSpacing
                Layout.fillWidth: true

                QQC2.ButtonGroup { id: filterGroup }

                QQC2.Button {
                    text: i18nc("@action:button filter", "All (%1)", notificationsModel.count)
                    checkable: true
                    checked: page.activeFilter === "all" && page.searchFilter.length === 0
                    QQC2.ButtonGroup.group: filterGroup
                    onClicked: {
                        page.searchFilter = "";
                        searchBox.text = "";
                        page.activeFilter = "all";
                    }
                }

                QQC2.Button {
                    text: i18nc("@action:button filter", "Can Reply")
                    icon.name: "mail-reply-sender"
                    checkable: true
                    checked: page.activeFilter === "repliable"
                    QQC2.ButtonGroup.group: filterGroup
                    onClicked: {
                        page.searchFilter = "";
                        searchBox.text = "";
                        page.activeFilter = "repliable";
                    }
                }

                QQC2.Button {
                    text: i18nc("@action:button filter", "Dismissable")
                    icon.name: "window-close"
                    checkable: true
                    checked: page.activeFilter === "dismissable"
                    QQC2.ButtonGroup.group: filterGroup
                    onClicked: {
                        page.searchFilter = "";
                        searchBox.text = "";
                        page.activeFilter = "dismissable";
                    }
                }

                Item { Layout.fillWidth: true }
            }
        }
    }

    // Empty state: informative placeholder when no notifications are present
    Kirigami.PlaceholderMessage {
        anchors.centerIn: parent
        width: parent.width - Kirigami.Units.gridUnit * 4
        visible: filteredNotificationsModel.count === 0
        icon.name: page.searchFilter.length > 0 ? "edit-none-symbolic" : "notifications"
        text: {
            if (page.searchFilter.length > 0) {
                return i18nc("@info:placeholder", "No matching notifications");
            }
            if (page.activeFilter === "repliable") {
                return i18nc("@info:placeholder", "No replyable notifications");
            }
            if (page.activeFilter === "dismissable") {
                return i18nc("@info:placeholder", "No dismissable notifications");
            }
            return i18nc("@info:placeholder", "No notifications");
        }
        explanation: {
            if (page.searchFilter.length > 0) {
                return i18nc("@info:placeholder", "No notifications match \"%1\".", page.searchFilter);
            }
            if (page.device) {
                return i18nc("@info:placeholder", "New notifications from %1 will appear here.", page.device.name);
            }
            return i18nc("@info:placeholder", "New notifications from your device will appear here.");
        }
    }

    // Main notification list: virtualized ListView grouped by application name
    ListView {
        id: notificationList
        anchors.fill: parent
        clip: true
        spacing: Kirigami.Units.mediumSpacing
        topMargin: Kirigami.Units.mediumSpacing
        bottomMargin: Kirigami.Units.largeSpacing
        leftMargin: Kirigami.Units.largeSpacing
        rightMargin: Kirigami.Units.largeSpacing

        model: filteredNotificationsModel

        // Section grouping by application name (e.g. WhatsApp, SMS, Email, etc.)
        section.property: "appName"
        section.criteria: ViewSection.FullString
        section.delegate: Component {
            Item {
                width: ListView.view.width - (ListView.view.leftMargin + ListView.view.rightMargin)
                height: sectionLayout.implicitHeight + Kirigami.Units.mediumSpacing * 2

                RowLayout {
                    id: sectionLayout
                    anchors.fill: parent
                    spacing: Kirigami.Units.mediumSpacing

                    Kirigami.Heading {
                        level: 4
                        text: section ? section : i18n("Applications")
                        font.bold: true
                        color: Kirigami.Theme.textColor
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        implicitHeight: 1
                        color: Kirigami.ColorUtils.tintWithAlpha(Kirigami.Theme.backgroundColor, Kirigami.Theme.textColor, 0.15)
                    }
                }
            }
        }

        delegate: QQC2.ItemDelegate {
            id: cardDelegate

            width: ListView.view.width - (ListView.view.leftMargin + ListView.view.rightMargin)
            implicitHeight: cardContent.implicitHeight + Kirigami.Units.largeSpacing * 2
            hoverEnabled: true
            focusPolicy: Qt.StrongFocus
            Accessible.role: Accessible.ListItem
            Accessible.name: model.title ? (model.appName + ": " + model.title) : model.appName

            property bool isReplying: false

            // Dismiss animation for smooth visual exit
            SequentialAnimation {
                id: dismissAnim

                NumberAnimation {
                    target: cardDelegate
                    property: "opacity"
                    to: 0
                    duration: Kirigami.Units.shortDuration
                    easing.type: Easing.InOutQuad
                }

                ScriptAction {
                    script: {
                        if (model.dbusInterface && model.dismissable) {
                            model.dbusInterface.dismiss();
                        }
                    }
                }
            }

            // Modern card background container
            background: Rectangle {
                radius: 16
                color: cardDelegate.hovered
                    ? Kirigami.ColorUtils.tintWithAlpha(Kirigami.Theme.backgroundColor, "#2a273f", 0.9)
                    : Kirigami.ColorUtils.tintWithAlpha(Kirigami.Theme.backgroundColor, "#232136", 0.85)
                border.color: Kirigami.ColorUtils.tintWithAlpha(Kirigami.Theme.backgroundColor, Kirigami.Theme.textColor, 0.12)
                border.width: 1

                Behavior on color {
                    ColorAnimation { duration: Kirigami.Units.shortDuration }
                }
            }

            // Card content layout
            contentItem: ColumnLayout {
                id: cardContent
                spacing: Kirigami.Units.mediumSpacing
                anchors.fill: parent
                anchors.margins: Kirigami.Units.largeSpacing

                // 1. Header Row: App Icon + App Name + "via KDE Connect" chip + Timestamp
                RowLayout {
                    Layout.fillWidth: true
                    spacing: Kirigami.Units.smallSpacing

                    Rectangle {
                        width: 22
                        height: 22
                        radius: 6
                        color: model.appName === "WhatsApp" ? "#25D366" : Kirigami.ColorUtils.tintWithAlpha(Kirigami.Theme.backgroundColor, Kirigami.Theme.textColor, 0.15)
                        clip: true

                        Kirigami.Icon {
                            anchors.centerIn: parent
                            width: 14
                            height: 14
                            source: model.appIcon ? model.appIcon : (model.appName === "WhatsApp" ? "im-whatsapp" : "notifications")
                            color: model.appName === "WhatsApp" ? "#ffffff" : Kirigami.Theme.textColor
                        }
                    }

                    QQC2.Label {
                        text: model.appName ? model.appName : i18n("Application")
                        font.bold: true
                        color: Kirigami.Theme.textColor
                    }

                    Rectangle {
                        radius: 10
                        implicitWidth: viaText.implicitWidth + 14
                        implicitHeight: viaText.implicitHeight + 6
                        color: Kirigami.ColorUtils.tintWithAlpha(Kirigami.Theme.backgroundColor, Kirigami.Theme.textColor, 0.08)

                        QQC2.Label {
                            id: viaText
                            anchors.centerIn: parent
                            text: "via KDE Connect"
                            font.pointSize: Math.max(8, Kirigami.Theme.smallFont.pointSize - 1)
                            opacity: 0.7
                        }
                    }

                    Item { Layout.fillWidth: true }

                    QQC2.Label {
                        text: i18nc("@info:status", "Baru saja")
                        font.pointSize: Math.max(8, Kirigami.Theme.smallFont.pointSize - 1)
                        opacity: 0.5
                    }
                }

                // 2. Middle Row: Avatar Circle (with Online Green Dot) + Sender / Message Body
                RowLayout {
                    Layout.fillWidth: true
                    spacing: Kirigami.Units.mediumSpacing

                    Item {
                        width: 44
                        height: 44
                        Layout.alignment: Qt.AlignTop

                        Rectangle {
                            anchors.fill: parent
                            radius: width / 2
                            color: Kirigami.ColorUtils.tintWithAlpha(Kirigami.Theme.backgroundColor, Kirigami.Theme.textColor, 0.15)
                            clip: true

                            Kirigami.Icon {
                                anchors.centerIn: parent
                                width: 30
                                height: 30
                                source: model.appIcon ? model.appIcon : "user-identity"
                            }
                        }

                        // Status dot (Green online indicator)
                        Rectangle {
                            width: 11
                            height: 11
                            radius: width / 2
                            anchors.bottom: parent.bottom
                            anchors.right: parent.right
                            anchors.margins: 1
                            color: "#22c55e"
                            border.color: Kirigami.Theme.backgroundColor
                            border.width: 2
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 3

                        Kirigami.Heading {
                            text: model.title ? model.title : (model.appName ? model.appName : i18n("Notification"))
                            level: 3
                            font.bold: true
                            Layout.fillWidth: true
                            elide: Text.ElideRight
                            color: Kirigami.Theme.textColor
                        }

                        QQC2.Label {
                            visible: model.notitext && model.notitext.length > 0
                            text: {
                                const t = model.title ? model.title : "";
                                const body = model.notitext ? model.notitext : "";
                                if (t.length > 0 && !body.startsWith(t)) {
                                    return t + ": " + body;
                                }
                                return body;
                            }
                            wrapMode: Text.WordWrap
                            Layout.fillWidth: true
                            opacity: 0.88
                            font.pointSize: Kirigami.Theme.defaultFont.pointSize
                        }
                    }
                }

                // 3. Actions Row: "Tandai sudah dibaca", "Senyapkan", and prominent blue "Balas" button
                RowLayout {
                    Layout.fillWidth: true
                    spacing: Kirigami.Units.smallSpacing
                    Layout.topMargin: Kirigami.Units.smallSpacing

                    QQC2.Button {
                        visible: model.dismissable
                        text: i18nc("@action:button", "Tandai sudah dibaca")
                        contentItem: QQC2.Label {
                            text: parent.text
                            font: parent.font
                            opacity: 0.9
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        background: Rectangle {
                            radius: 12
                            color: parent.down ? Kirigami.ColorUtils.tintWithAlpha(Kirigami.Theme.backgroundColor, Kirigami.Theme.textColor, 0.18)
                                               : (parent.hovered ? Kirigami.ColorUtils.tintWithAlpha(Kirigami.Theme.backgroundColor, Kirigami.Theme.textColor, 0.12)
                                                                 : Kirigami.ColorUtils.tintWithAlpha(Kirigami.Theme.backgroundColor, Kirigami.Theme.textColor, 0.08))
                            border.color: Kirigami.ColorUtils.tintWithAlpha(Kirigami.Theme.backgroundColor, Kirigami.Theme.textColor, 0.1)
                            border.width: 1
                        }
                        onClicked: dismissAnim.start()
                    }

                    QQC2.Button {
                        visible: model.dismissable
                        text: i18nc("@action:button", "Senyapkan")
                        contentItem: QQC2.Label {
                            text: parent.text
                            font: parent.font
                            opacity: 0.9
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        background: Rectangle {
                            radius: 12
                            color: parent.down ? Kirigami.ColorUtils.tintWithAlpha(Kirigami.Theme.backgroundColor, Kirigami.Theme.textColor, 0.18)
                                               : (parent.hovered ? Kirigami.ColorUtils.tintWithAlpha(Kirigami.Theme.backgroundColor, Kirigami.Theme.textColor, 0.12)
                                                                 : Kirigami.ColorUtils.tintWithAlpha(Kirigami.Theme.backgroundColor, Kirigami.Theme.textColor, 0.08))
                            border.color: Kirigami.ColorUtils.tintWithAlpha(Kirigami.Theme.backgroundColor, Kirigami.Theme.textColor, 0.1)
                            border.width: 1
                        }
                        onClicked: dismissAnim.start()
                    }

                    Item { Layout.fillWidth: true }

                    QQC2.Button {
                        visible: model.repliable
                        text: i18nc("@action:button", "Balas")
                        font.bold: true
                        contentItem: QQC2.Label {
                            text: parent.text
                            font: parent.font
                            color: "#ffffff"
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        background: Rectangle {
                            radius: 12
                            color: parent.down ? "#1d4ed8" : (parent.hovered ? "#3b82f6" : "#2563eb")
                            implicitWidth: 78
                            implicitHeight: 34
                        }
                        onClicked: {
                            cardDelegate.isReplying = !cardDelegate.isReplying;
                            if (cardDelegate.isReplying) {
                                inlineReplyField.forceActiveFocus();
                            }
                        }
                    }
                }

                // 4. Card 2: Quick Reply Card (Balasan Cepat ke [Nama])
                Rectangle {
                    visible: cardDelegate.isReplying
                    Layout.fillWidth: true
                    Layout.topMargin: Kirigami.Units.smallSpacing
                    radius: 14
                    color: Kirigami.ColorUtils.tintWithAlpha(Kirigami.Theme.backgroundColor, Kirigami.Theme.textColor, 0.04)
                    border.color: Kirigami.ColorUtils.tintWithAlpha(Kirigami.Theme.backgroundColor, Kirigami.Theme.textColor, 0.1)
                    border.width: 1
                    implicitHeight: replyCardContent.implicitHeight + Kirigami.Units.mediumSpacing * 2

                    ColumnLayout {
                        id: replyCardContent
                        anchors.fill: parent
                        anchors.margins: Kirigami.Units.mediumSpacing
                        spacing: Kirigami.Units.smallSpacing

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 6

                            Rectangle {
                                width: 7
                                height: 7
                                radius: width / 2
                                color: "#3b82f6"
                            }

                            QQC2.Label {
                                text: i18nc("@title:group", "Balasan Cepat ke %1", model.title ? model.title : model.appName)
                                font.bold: true
                                Layout.fillWidth: true
                            }

                            QQC2.Label {
                                text: model.appName ? model.appName : ""
                                opacity: 0.5
                                font.pointSize: Math.max(8, Kirigami.Theme.smallFont.pointSize - 1)
                            }
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            implicitHeight: Math.max(inlineReplyField.implicitHeight + 8, 40)
                            radius: 12
                            color: Kirigami.ColorUtils.tintWithAlpha(Kirigami.Theme.backgroundColor, Kirigami.Theme.textColor, 0.06)
                            border.color: inlineReplyField.activeFocus ? "#3b82f6" : Kirigami.ColorUtils.tintWithAlpha(Kirigami.Theme.backgroundColor, Kirigami.Theme.textColor, 0.12)
                            border.width: 1

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: Kirigami.Units.mediumSpacing
                                anchors.rightMargin: 6
                                spacing: Kirigami.Units.smallSpacing

                                QQC2.TextField {
                                    id: inlineReplyField
                                    Layout.fillWidth: true
                                    placeholderText: i18nc("@info:placeholder", "Iya, ada apa?")
                                    background: null
                                    onAccepted: sendBtn.clicked()
                                    Keys.onEscapePressed: {
                                        text = "";
                                        cardDelegate.isReplying = false;
                                    }
                                }

                                QQC2.Button {
                                    id: sendBtn
                                    text: i18nc("@action:button", "Kirim")
                                    font.bold: true
                                    enabled: inlineReplyField.text.trim().length > 0
                                    contentItem: QQC2.Label {
                                        text: parent.text
                                        font: parent.font
                                        color: parent.enabled ? "#ffffff" : "rgba(255,255,255,0.4)"
                                        horizontalAlignment: Text.AlignHCenter
                                        verticalAlignment: Text.AlignVCenter
                                    }
                                    background: Rectangle {
                                        radius: 10
                                        implicitWidth: 64
                                        implicitHeight: 30
                                        color: parent.enabled
                                            ? (parent.down ? "#1d4ed8" : (parent.hovered ? "#3b82f6" : "#2563eb"))
                                            : Kirigami.ColorUtils.tintWithAlpha(Kirigami.Theme.backgroundColor, Kirigami.Theme.textColor, 0.08)
                                    }
                                    onClicked: {
                                        if (model.dbusInterface) {
                                            model.dbusInterface.sendReply(inlineReplyField.text.trim());
                                        }
                                        inlineReplyField.text = "";
                                        cardDelegate.isReplying = false;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
