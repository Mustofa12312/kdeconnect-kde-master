/*
 * SPDX-FileCopyrightText: 2026 KDE Connect contributors
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard
import org.kde.kdeconnect

Kirigami.ScrollablePage {
    id: root

    // Signal emitted when the user activates a device card.
    signal deviceActivated(deviceId: string)

    title: i18nc("@title:window", "Overview")

    // Local DevicesModel: a thin D-Bus proxy reflecting kdeconnectd state.
    // kdeconnectd remains the authoritative source; this model is read-only.
    DevicesModel {
        id: devicesModel
    }

    ColumnLayout {
        spacing: 0
        width: parent.width

        // --- Loading state ---
        QQC2.BusyIndicator {
            Layout.alignment: Qt.AlignHCenter
            Layout.topMargin: Kirigami.Units.gridUnit * 4
            visible: devicesModel.busy
        }

        // --- Empty state: no paired or reachable devices ---
        Kirigami.PlaceholderMessage {
            Layout.fillWidth: true
            Layout.topMargin: Kirigami.Units.gridUnit * 2
            Layout.bottomMargin: Kirigami.Units.gridUnit
            Layout.leftMargin: Kirigami.Units.gridUnit * 4
            Layout.rightMargin: Kirigami.Units.gridUnit * 4
            visible: !devicesModel.busy && deviceRepeater.count === 0
            icon.name: "network-connect"
            text: i18nc("@info placeholder", "No devices found")
            explanation: i18nc(
                "@info placeholder explanation",
                "Make sure KDE Connect is running on your other device and that both are on the same network."
            )

            helpfulAction: Kirigami.Action {
                text: i18nc("@action:button", "Refresh")
                icon.name: "view-refresh-symbolic"
                onTriggered: DaemonDbusInterface.forceOnNetworkChange()
            }
        }

        // --- Section heading for the device grid ---
        Kirigami.Heading {
            Layout.fillWidth: true
            Layout.topMargin: Kirigami.Units.gridUnit
            Layout.leftMargin: Kirigami.Units.largeSpacing
            Layout.bottomMargin: Kirigami.Units.smallSpacing
            text: i18nc("@title:group", "Devices")
            level: 2
            visible: !devicesModel.busy && deviceRepeater.count > 0
        }

        // --- Device cards ---
        Repeater {
            id: deviceRepeater

            model: DevicesSortProxyModel {
                sourceModel: devicesModel
            }

            delegate: DeviceCard {
                required property DeviceDbusInterface device
                required property string name
                required property string iconName
                required property int status

                Layout.fillWidth: true
                Layout.leftMargin: Kirigami.Units.largeSpacing
                Layout.rightMargin: Kirigami.Units.largeSpacing
                Layout.topMargin: Kirigami.Units.smallSpacing

                cardDevice: device
                cardName: name
                cardIconName: iconName

                onOpenRequested: root.deviceActivated(device.id())
            }
        }

        Item {
            Layout.minimumHeight: Kirigami.Units.gridUnit
            Layout.fillWidth: true
        }

        // --- App store links: only when no devices are visible ---
        FormCard.FormHeader {
            Layout.fillWidth: true
            visible: !devicesModel.busy && deviceRepeater.count === 0
            title: i18nc("@title:context", "KDE Connect Android App")
        }

        FormCard.FormCard {
            Layout.fillWidth: true
            visible: !devicesModel.busy && deviceRepeater.count === 0

            FormCard.FormButtonDelegate {
                text: i18nc("Android Application Store", "F-Droid")
                onClicked: Qt.openUrlExternally("https://f-droid.org/repository/browse/?fdid=org.kde.kdeconnect_tp")
            }

            FormCard.FormDelegateSeparator {}

            FormCard.FormButtonDelegate {
                text: i18nc("Android Application Store", "Play Store")
                onClicked: Qt.openUrlExternally("https://play.google.com/store/apps/details?id=org.kde.kdeconnect_tp")
            }
        }

        FormCard.FormHeader {
            Layout.fillWidth: true
            visible: !devicesModel.busy && deviceRepeater.count === 0
            title: i18nc("@title:context", "KDE Connect iOS App")
        }

        FormCard.FormCard {
            Layout.fillWidth: true
            visible: !devicesModel.busy && deviceRepeater.count === 0

            FormCard.FormButtonDelegate {
                text: i18n("App Store")
                onClicked: Qt.openUrlExternally("https://apps.apple.com/us/app/kde-connect/id1580245991")
            }
        }

        Item {
            Layout.fillWidth: true
            Layout.minimumHeight: Kirigami.Units.gridUnit
        }
    }

    // -------------------------------------------------------------------------
    // DeviceCard: extracted as a named component so the Repeater delegate can
    // use plain properties without pragma ComponentBehavior: Bound restrictions.
    // All state comes from real D-Bus interfaces; no mock values are used.
    // -------------------------------------------------------------------------
    component DeviceCard: QQC2.ItemDelegate {
        id: card

        // Properties set by the Repeater delegate
        property DeviceDbusInterface cardDevice
        property string cardName
        property string cardIconName

        // Emitted when the user wants to navigate to the full DevicePage
        signal openRequested()

        hoverEnabled: true
        focusPolicy: Qt.StrongFocus
        Accessible.role: Accessible.ListItem
        Accessible.name: cardName

        onClicked: {
            if (cardDevice.isPaired && cardDevice.isReachable) {
                card.openRequested()
            }
        }

        background: Rectangle {
            radius: Kirigami.Units.smallSpacing
            color: card.hovered ? Kirigami.Theme.hoverColor : Kirigami.Theme.backgroundColor
            border.color: Kirigami.ColorUtils.tintWithAlpha(Kirigami.Theme.backgroundColor, Kirigami.Theme.textColor, 0.15)
            border.width: 1

            Behavior on color {
                ColorAnimation { duration: Kirigami.Units.shortDuration }
            }
        }

        // Battery helper - polls real D-Bus; hasBattery is false when unavailable
        BatteryInfo {
            id: batteryInfo
            device: card.cardDevice
        }

        // Connectivity helper - available is false when plugin is absent
        ConnectivityInfo {
            id: connectivityInfo
            device: card.cardDevice
        }

        NotificationsModel {
            id: cardNotificationsModel
            deviceId: (card.cardDevice && card.cardDevice.isPaired && card.cardDevice.isReachable) ? card.cardDevice.id() : ""
        }

        contentItem: ColumnLayout {
            spacing: Kirigami.Units.smallSpacing

            // Header: icon + name + status dot
            RowLayout {
                spacing: Kirigami.Units.largeSpacing
                Layout.fillWidth: true

                Kirigami.Icon {
                    source: card.cardIconName
                    implicitWidth: Kirigami.Units.iconSizes.medium
                    implicitHeight: Kirigami.Units.iconSizes.medium
                    Layout.alignment: Qt.AlignVCenter
                }

                ColumnLayout {
                    spacing: 0
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignVCenter

                    Kirigami.Heading {
                        text: card.cardName
                        level: 4
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }

                    QQC2.Label {
                        font: Kirigami.Theme.smallFont
                        elide: Text.ElideRight
                        Layout.fillWidth: true

                        // All four paired x reachable states are covered.
                        // Disconnected devices are never labeled as connected.
                        text: {
                            const p = card.cardDevice.isPaired;
                            const r = card.cardDevice.isReachable;
                            if (p && r) {
                                return i18nc("@info device status", "Connected");
                            } else if (p && !r) {
                                return i18nc("@info device status", "Remembered - not reachable");
                            } else if (!p && r) {
                                return i18nc("@info device status", "Available - not paired");
                            }
                            return i18nc("@info device status", "Unavailable");
                        }

                        color: {
                            const p = card.cardDevice.isPaired;
                            const r = card.cardDevice.isReachable;
                            if (p && r) {
                                return Kirigami.Theme.positiveTextColor;
                            } else if (!p && r) {
                                return Kirigami.Theme.neutralTextColor;
                            }
                            return Kirigami.Theme.disabledTextColor;
                        }
                    }
                }

                // Status indicator dot
                Rectangle {
                    implicitWidth: Kirigami.Units.smallSpacing * 2
                    implicitHeight: Kirigami.Units.smallSpacing * 2
                    radius: width / 2
                    Layout.alignment: Qt.AlignVCenter
                    Accessible.ignored: true

                    color: {
                        const p = card.cardDevice.isPaired;
                        const r = card.cardDevice.isReachable;
                        if (p && r) {
                            return Kirigami.Theme.positiveTextColor;
                        } else if (!p && r) {
                            return Kirigami.Theme.neutralTextColor;
                        }
                        return Kirigami.Theme.disabledTextColor;
                    }
                }
            }

            // Battery row: hidden when plugin is unavailable or charge unknown
            RowLayout {
                visible: batteryInfo.hasBattery
                spacing: Kirigami.Units.smallSpacing
                Layout.topMargin: Kirigami.Units.smallSpacing

                Kirigami.Icon {
                    source: batteryInfo.iconName
                    implicitWidth: Kirigami.Units.iconSizes.small
                    implicitHeight: Kirigami.Units.iconSizes.small
                    Layout.alignment: Qt.AlignVCenter
                    Accessible.ignored: true
                }

                QQC2.Label {
                    font: Kirigami.Theme.smallFont
                    text: batteryInfo.displayString
                    Layout.alignment: Qt.AlignVCenter
                }

                // Connectivity signal chip (shown inline with battery when both present)
                Kirigami.Icon {
                    visible: connectivityInfo.available
                    source: connectivityInfo.iconName
                    implicitWidth: Kirigami.Units.iconSizes.small
                    implicitHeight: Kirigami.Units.iconSizes.small
                    Layout.alignment: Qt.AlignVCenter
                    Accessible.ignored: true
                }

                QQC2.Label {
                    visible: connectivityInfo.available
                    font: Kirigami.Theme.smallFont
                    text: connectivityInfo.networkType
                    Layout.alignment: Qt.AlignVCenter
                }
            }

            // Connectivity row: shown only when battery plugin is absent
            RowLayout {
                visible: !batteryInfo.hasBattery && connectivityInfo.available
                spacing: Kirigami.Units.smallSpacing
                Layout.topMargin: Kirigami.Units.smallSpacing

                Kirigami.Icon {
                    source: connectivityInfo.iconName
                    implicitWidth: Kirigami.Units.iconSizes.small
                    implicitHeight: Kirigami.Units.iconSizes.small
                    Layout.alignment: Qt.AlignVCenter
                    Accessible.ignored: true
                }

                QQC2.Label {
                    font: Kirigami.Theme.smallFont
                    text: connectivityInfo.displayString
                    Layout.alignment: Qt.AlignVCenter
                }
            }

            // Active notifications banner on device card (Modern Glassmorphic Style)
            Rectangle {
                visible: cardNotificationsModel.count > 0 && card.cardDevice.isPaired && card.cardDevice.isReachable
                Layout.fillWidth: true
                Layout.topMargin: Kirigami.Units.smallSpacing
                implicitHeight: notiRow.implicitHeight + Kirigami.Units.smallSpacing * 2
                radius: 12
                color: Kirigami.ColorUtils.tintWithAlpha(Kirigami.Theme.backgroundColor, "#232136", 0.8)
                border.color: Kirigami.ColorUtils.tintWithAlpha(Kirigami.Theme.backgroundColor, Kirigami.Theme.textColor, 0.12)
                border.width: 1

                RowLayout {
                    id: notiRow
                    anchors.fill: parent
                    anchors.margins: Kirigami.Units.smallSpacing
                    spacing: Kirigami.Units.smallSpacing

                    Rectangle {
                        width: 24
                        height: 24
                        radius: 6
                        color: "#25D366"
                        clip: true

                        Kirigami.Icon {
                            anchors.centerIn: parent
                            width: 15
                            height: 15
                            source: "notifications"
                            color: "#ffffff"
                        }
                    }

                    QQC2.Label {
                        text: i18n("%1 notifikasi aktif", cardNotificationsModel.count)
                        font.bold: true
                        Layout.fillWidth: true
                        color: Kirigami.Theme.textColor
                    }

                    Rectangle {
                        radius: 10
                        implicitWidth: viaTextOverview.implicitWidth + 12
                        implicitHeight: viaTextOverview.implicitHeight + 4
                        color: Kirigami.ColorUtils.tintWithAlpha(Kirigami.Theme.backgroundColor, Kirigami.Theme.textColor, 0.08)

                        QQC2.Label {
                            id: viaTextOverview
                            anchors.centerIn: parent
                            text: "via KDE Connect"
                            font.pointSize: Math.max(8, Kirigami.Theme.smallFont.pointSize - 1)
                            opacity: 0.7
                        }
                    }

                    QQC2.Button {
                        text: i18n("Buka")
                        font.bold: true
                        contentItem: QQC2.Label {
                            text: parent.text
                            font: parent.font
                            color: "#ffffff"
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        background: Rectangle {
                            radius: 10
                            color: parent.down ? "#1d4ed8" : (parent.hovered ? "#3b82f6" : "#2563eb")
                            implicitWidth: 62
                            implicitHeight: 28
                        }
                        onClicked: card.openRequested()
                    }
                }
            }

            // Incoming pairing request row
            RowLayout {
                visible: card.cardDevice.isPairRequestedByPeer
                spacing: Kirigami.Units.smallSpacing
                Layout.topMargin: Kirigami.Units.smallSpacing

                QQC2.Label {
                    text: i18nc("@info", "Pairing request received")
                    font: Kirigami.Theme.smallFont
                    color: Kirigami.Theme.neutralTextColor
                    Layout.fillWidth: true
                }

                QQC2.Button {
                    text: i18nc("@action:button Accept pairing request", "Accept")
                    icon.name: "dialog-ok"
                    onClicked: card.cardDevice.acceptPairing()
                }

                QQC2.Button {
                    text: i18nc("@action:button Reject pairing request", "Reject")
                    icon.name: "dialog-cancel"
                    onClicked: card.cardDevice.cancelPairing()
                }
            }

            // Quick actions row
            RowLayout {
                spacing: Kirigami.Units.smallSpacing
                Layout.topMargin: Kirigami.Units.smallSpacing

                QQC2.Button {
                    visible: card.cardDevice.isReachable
                        && !card.cardDevice.isPaired
                        && !card.cardDevice.isPairRequested
                        && !card.cardDevice.isPairRequestedByPeer
                    text: i18nc("@action:button Request pairing", "Pair")
                    icon.name: "network-connect"
                    onClicked: card.cardDevice.requestPairing()
                }

                QQC2.Button {
                    visible: card.cardDevice.isPaired && card.cardDevice.isReachable
                    text: i18nc("@action:button Open device details", "Open")
                    icon.name: "go-next-symbolic"
                    onClicked: card.openRequested()
                }

                Item { Layout.fillWidth: true }

                QQC2.ToolButton {
                    visible: card.cardDevice.isPaired && card.cardDevice.isReachable
                    text: i18nc("@action:button", "Send Ping")
                    icon.name: "hands-free"
                    display: QQC2.AbstractButton.IconOnly
                    QQC2.ToolTip.text: text
                    QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
                    QQC2.ToolTip.visible: hovered
                    onClicked: card.cardDevice.pluginCall("ping", "sendPing")
                }
            }
        }

        topPadding: Kirigami.Units.largeSpacing
        bottomPadding: Kirigami.Units.largeSpacing
        leftPadding: Kirigami.Units.largeSpacing
        rightPadding: Kirigami.Units.largeSpacing
    }
}
