/**
 * SPDX-FileCopyrightText: 2013 Albert Vaca <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include "core/kdeconnectplugin.h"

/*
 * This class is just a proxy for NotificationsDbusInterface
 * because it can not inherit from QDBusAbstractAdaptor and
 * KdeConnectPlugin at the same time (both are QObject)
 */
class NotificationsDbusInterface;
class NotificationsListener;

class SendNotificationsPlugin : public KdeConnectPlugin
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.kdeconnect.device.sendnotifications")

public:
    explicit SendNotificationsPlugin(QObject *parent, const QVariantList &args);
    ~SendNotificationsPlugin() override;

    void receivePacket(const NetworkPacket &np) override;
    QString dbusPath() const override;

public Q_SLOTS:
    Q_SCRIPTABLE QString sendPrompt(const QString &title, const QString &text, const QStringList &actions);

Q_SIGNALS:
    Q_SCRIPTABLE void actionInvoked(const QString &id, const QString &action);

protected:
    NotificationsListener *notificationsListener;
};
