/*
 * SPDX-FileCopyrightText: 2019 Nicolas Fella <nicolas.fella@gmx.de>
 * SPDX-FileCopyrightText: 2026 KDE Connect contributors
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kitemmodels as KItemModels
import org.kde.kdeconnect

Kirigami.ScrollablePage {
    id: root

    property string device
    property string filterString
    readonly property bool isPluginSettingsPage: true

    title: i18n("Plugin Settings")

    // Map plugin IDs to category labels shown as section headers.
    // All categorisation is in QML; PluginModel and its data are unchanged.
    // Plugins absent from this map fall under "Other".
    readonly property var pluginCategoryMap: ({
        "kdeconnect_clipboard":          i18nc("@title:group plugin category", "Data Sync"),
        "kdeconnect_contacts":           i18nc("@title:group plugin category", "Data Sync"),
        "kdeconnect_notifications":      i18nc("@title:group plugin category", "Notifications"),
        "kdeconnect_sendnotifications":  i18nc("@title:group plugin category", "Notifications"),
        "kdeconnect_telephony":          i18nc("@title:group plugin category", "Notifications"),
        "kdeconnect_mmtelephony":        i18nc("@title:group plugin category", "Notifications"),
        "kdeconnect_share":              i18nc("@title:group plugin category", "Files"),
        "kdeconnect_sftp":               i18nc("@title:group plugin category", "Files"),
        "kdeconnect_remotecontrol":      i18nc("@title:group plugin category", "Remote Control"),
        "kdeconnect_remotekeyboard":     i18nc("@title:group plugin category", "Remote Control"),
        "kdeconnect_mprisremote":        i18nc("@title:group plugin category", "Remote Control"),
        "kdeconnect_remotesystemvolume": i18nc("@title:group plugin category", "Remote Control"),
        "kdeconnect_remotecommands":     i18nc("@title:group plugin category", "Remote Control"),
        "kdeconnect_presenter":          i18nc("@title:group plugin category", "Remote Control"),
        "kdeconnect_ping":               i18nc("@title:group plugin category", "Device Actions"),
        "kdeconnect_findmyphone":        i18nc("@title:group plugin category", "Device Actions"),
        "kdeconnect_lockdevice":         i18nc("@title:group plugin category", "Device Actions"),
        "kdeconnect_findthisdevice":     i18nc("@title:group plugin category", "Device Actions"),
        "kdeconnect_virtualmonitor":     i18nc("@title:group plugin category", "Device Actions"),
        "kdeconnect_battery":            i18nc("@title:group plugin category", "Status"),
        "kdeconnect_connectivity_report":i18nc("@title:group plugin category", "Status"),
        "kdeconnect_mpris":              i18nc("@title:group plugin category", "Status"),
        "kdeconnect_shareinputdevices":  i18nc("@title:group plugin category", "Input Sharing"),
        "kdeconnect_digitizer":          i18nc("@title:group plugin category", "Input Sharing"),
        "kdeconnect_pausemusic":         i18nc("@title:group plugin category", "Automation"),
        "kdeconnect_screensaver_inhibit":i18nc("@title:group plugin category", "Automation"),
        "kdeconnect_runcommand":         i18nc("@title:group plugin category", "Automation"),
    })

    function pluginCategory(pluginId) {
        return pluginCategoryMap[pluginId] ?? i18nc("@title:group plugin category", "Other");
    }

    function openConfiguration(pluginName) {
        let source = pluginModel.pluginSource(pluginName);
        console.log("source", source);
        return devicePage = pageStack.push(source, {
            title: pluginModel.pluginDisplayName(pluginName),
            device: root.device,
        });
    }

    header: Control {
        topPadding: Kirigami.Units.smallSpacing
        bottomPadding: Kirigami.Units.smallSpacing
        leftPadding: Kirigami.Units.smallSpacing
        rightPadding: Kirigami.Units.smallSpacing

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

        contentItem: Kirigami.SearchField {
            id: searchField
            onTextChanged: root.filterString = text;
            autoAccept: false
            focus: true
            Keys.onDownPressed: event => {
                pluginList.currentIndex = 0;
                event.accepted = false; // Pass to KeyNavigation.down
            }
            KeyNavigation.down: pluginList
        }
    }

    ListView {
        id: pluginList
        Accessible.role: Accessible.List

        model: KItemModels.KSortFilterProxyModel {
            filterString: root.filterString
            filterRoleName: "name"
            filterCaseSensitivity: Qt.CaseInsensitive

            sourceModel: PluginModel {
                id: pluginModel
                deviceId: root.device
            }
        }

        // Group by the category label derived from pluginId.
        // The section property is evaluated per-row in the delegate via pluginId.
        section.property: "pluginId"
        section.criteria: ViewSection.FullString
        section.delegate: Kirigami.ListSectionHeader {
            required property string section
            width: ListView.view.width
            // Resolve pluginId -> category label via the parent page function.
            text: root.pluginCategory(section)
        }

        delegate: ItemDelegate {
            id: pluginDelegate

            width: ListView.view.width

            required property var model

            checkable: true
            checked: model.isChecked

            KeyNavigation.tab: settingsButton
            KeyNavigation.right: settingsButton

            Accessible.description: model.description
            Accessible.role: Accessible.CheckBox

            onToggled: {
                pluginList.currentIndex = model.index
                model.isChecked = !model.isChecked
            }

            contentItem: RowLayout {
                CheckBox {
                    id: serviceCheck
                    Layout.alignment: Qt.AlignVCenter
                    checked: pluginDelegate.model.isChecked

                    activeFocusOnTab: false
                    onToggled: {
                        pluginDelegate.model.isChecked = checked
                    }

                    Accessible.ignored: true
                }

                Kirigami.Icon {
                    source: pluginDelegate.model.iconName
                }

                ColumnLayout {
                    spacing: 0
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignVCenter

                    Label {
                        Layout.fillWidth: true
                        text: pluginDelegate.model.name
                        elide: Text.ElideRight
                    }

                    Label {
                        Layout.fillWidth: true
                        text: pluginDelegate.model.description
                        elide: Text.ElideRight
                        font: Kirigami.Theme.smallFont
                        opacity: 0.7
                    }
                }

                ToolButton {
                    id: settingsButton

                    visible: pluginDelegate.model.configSource != ""

                    icon.name: "settings-configure"

                    onClicked: {
                        pageStack.push(pluginDelegate.model.configSource, {
                            title: pluginDelegate.model.name,
                            device: root.device,
                        });
                    }
                }
            }
        }
    }
}
