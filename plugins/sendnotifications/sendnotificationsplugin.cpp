/**
 * SPDX-FileCopyrightText: 2013 Albert Vaca <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "sendnotificationsplugin.h"
#include "notifyingapplication.h"

#include <QDBusConnection>
#include <QDBusMessage>
#include <QRandomGenerator>

#include "core/device.h"
#include "notificationslistener.h"

#if defined(Q_OS_UNIX) && !defined(Q_OS_DARWIN)
#include "dbusnotificationslistener.h"
#elif defined(Q_OS_WIN)
#include "windowsnotificationslistener.h"
#include <windows.h>

#include <appmodel.h> // GetCurrentPackageFullName
#endif
#include <KPluginFactory>

#define PACKET_TYPE_NOTIFICATION_ACTION QStringLiteral("kdeconnect.notification.action")

K_PLUGIN_CLASS_WITH_JSON(SendNotificationsPlugin, "kdeconnect_sendnotifications.json")

SendNotificationsPlugin::SendNotificationsPlugin(QObject *parent, const QVariantList &args)
    : KdeConnectPlugin(parent, args)
{
    qRegisterMetaType<NotifyingApplication>("NotifyingApplication");
#if defined(Q_OS_UNIX) && !defined(Q_OS_DARWIN)
    notificationsListener = new DBusNotificationsListener(this);
#elif defined(Q_OS_WIN)
    std::uint32_t bufferLength = 100;
    std::array<wchar_t, 100> buffer;
    if (GetCurrentPackageFullName(&bufferLength, buffer.data()) == ERROR_SUCCESS) {
        notificationsListener = new WindowsNotificationsListener(this);
    }
#endif
}

SendNotificationsPlugin::~SendNotificationsPlugin()
{
    delete notificationsListener;
}

QString SendNotificationsPlugin::dbusPath() const
{
    return QStringLiteral("/modules/kdeconnect/devices/") + device()->id() + QStringLiteral("/sendnotifications");
}

void SendNotificationsPlugin::receivePacket(const NetworkPacket &np)
{
    if (np.type() == PACKET_TYPE_NOTIFICATION_ACTION) {
        const QString id = np.get<QString>(QStringLiteral("id"));
        const QString action = np.get<QString>(QStringLiteral("action"));

        Q_EMIT actionInvoked(id, action);

        bool ok = false;
        uint uintId = id.toUInt(&ok);
        if (ok) {
            QDBusMessage msg = QDBusMessage::createSignal(
                QStringLiteral("/org/freedesktop/Notifications"),
                QStringLiteral("org.freedesktop.Notifications"),
                QStringLiteral("ActionInvoked")
            );
            msg << uintId << action;
            QDBusConnection::sessionBus().send(msg);
        }
    }
}

QString SendNotificationsPlugin::sendPrompt(const QString &title, const QString &text, const QStringList &actions)
{
    const QString promptId = QString::number(QRandomGenerator::global()->generate());
    const QString ticker = title + QStringLiteral(": ") + text;
    NetworkPacket np(PACKET_TYPE_NOTIFICATION,
                     {
                         {QStringLiteral("id"), promptId},
                         {QStringLiteral("appName"), QStringLiteral("Antigravity Authorization")},
                         {QStringLiteral("title"), title},
                         {QStringLiteral("ticker"), ticker},
                         {QStringLiteral("text"), text},
                         {QStringLiteral("actions"), actions},
                         {QStringLiteral("isPrompt"), true},
                         {QStringLiteral("isClearable"), false},
                         {QStringLiteral("silent"), false},
                     });
    sendPacket(np);
    return promptId;
}

#include "moc_sendnotificationsplugin.cpp"
#include "sendnotificationsplugin.moc"
