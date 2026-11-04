/**
 * SPDX-FileCopyrightText: 2013 Albert Vaca <albertvaka@gmail.com>
 * SPDX-FileCopyrightText: 2018 Simon Redman <simon@ergotech.com>
 * SPDX-FileCopyrightText: 2026 KDE Connect contributors
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <QPointer>

#include <KNotification>

#include <core/kdeconnectplugin.h>

#define PACKET_TYPE_TELEPHONY QStringLiteral("kdeconnect.telephony")
#define PACKET_TYPE_TELEPHONY_REQUEST_MUTE QStringLiteral("kdeconnect.telephony.request_mute")

class SendReplyDialog;

class TelephonyPlugin : public KdeConnectPlugin
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.kdeconnect.device.telephony")

public:
    using KdeConnectPlugin::KdeConnectPlugin;

    void receivePacket(const NetworkPacket &np) override;
    QString dbusPath() const override;

Q_SIGNALS:
    Q_SCRIPTABLE void callReceived(const QString &event, const QString &number, const QString contactName);

private Q_SLOTS:
    void sendMutePacket();

private:
    void createNotification(const NetworkPacket &np);
    void showCallOverlay(const QString &contactName, const QString &phoneNumber, const QByteArray &phoneThumbnail);

    QPointer<KNotification> m_currentCallNotification;
    QPointer<SendReplyDialog> m_currentCallDialog;
};
