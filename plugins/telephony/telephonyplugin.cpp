/**
 * SPDX-FileCopyrightText: 2013 Albert Vaca <albertvaka@gmail.com>
 * SPDX-FileCopyrightText: 2018 Simon Redman <simon@ergotech.com>
 * SPDX-FileCopyrightText: 2026 KDE Connect contributors
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "telephonyplugin.h"
#include "sendreplydialog.h"

#include <KLocalizedString>
#include <QDBusArgument>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusMessage>
#include <QDBusObjectPath>
#include <QDBusReply>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QTemporaryFile>
#include <QTextStream>

#include "plugin_telephony_debug.h"
#include <KNotification>
#include <KPluginFactory>

K_PLUGIN_CLASS_WITH_JSON(TelephonyPlugin, "kdeconnect_telephony.json")

void TelephonyPlugin::createNotification(const NetworkPacket &np)
{
    const QString event = np.get<QString>(QStringLiteral("event"));
    const QString phoneNumber = np.get<QString>(QStringLiteral("phoneNumber"), i18n("unknown number"));
    const QString contactName = np.get<QString>(QStringLiteral("contactName"), phoneNumber);
    const QByteArray phoneThumbnail = QByteArray::fromBase64(np.get<QByteArray>(QStringLiteral("phoneThumbnail"), ""));

    QString content, type, icon;

    if (event == QLatin1String("ringing")) {
        type = QStringLiteral("callReceived");
        icon = QStringLiteral("call-start");
        content = i18n("Incoming call from %1", contactName);
        ensureWireplumberConfig();
        connectBluetoothDevice();
    } else if (event == QLatin1String("missedCall")) {
        type = QStringLiteral("missedCall");
        icon = QStringLiteral("call-start");
        content = i18n("Missed call from %1", contactName);
    } else if (event == QLatin1String("talking")) {
        if (m_currentCallDialog) {
            m_currentCallDialog->setCallConnected();
        }
        if (m_currentCallNotification) {
            m_currentCallNotification->close();
        }
        return;
    }

    Q_EMIT callReceived(type, phoneNumber, contactName);

    qCDebug(KDECONNECT_PLUGIN_TELEPHONY) << "Creating notification with type:" << type;

    // In a GUI desktop session, display our custom glassmorphic overlay directly below the clock
    if (!qEnvironmentVariableIsEmpty("DISPLAY") || !qEnvironmentVariableIsEmpty("WAYLAND_DISPLAY")) {
        if (event == QLatin1String("ringing")) {
            showCallOverlay(contactName, phoneNumber, phoneThumbnail);
            return;
        }
    }

    if (!m_currentCallNotification) {
        m_currentCallNotification = new KNotification(type, KNotification::Persistent);
    }

    if (!phoneThumbnail.isEmpty()) {
        QPixmap photo;
        photo.loadFromData(phoneThumbnail, "JPEG");
        m_currentCallNotification->setPixmap(photo);
    } else {
        m_currentCallNotification->setIconName(icon);
    }
    m_currentCallNotification->setComponentName(QStringLiteral("kdeconnect"));
    m_currentCallNotification->setTitle(device()->name());
    m_currentCallNotification->setText(content);

    if (event == QLatin1String("ringing")) {
        m_currentCallNotification->clearActions();
        KNotificationAction *muteAction = m_currentCallNotification->addAction(i18n("Mute Call"));
        connect(muteAction, &KNotificationAction::activated, this, &TelephonyPlugin::sendMutePacket);
    }

    m_currentCallNotification->sendEvent();
}

void TelephonyPlugin::showCallOverlay(const QString &contactName, const QString &phoneNumber, const QByteArray &phoneThumbnail)
{
    if (m_currentCallDialog) {
        m_currentCallDialog->close();
        m_currentCallDialog = nullptr;
    }

    QString photoPath;
    if (!phoneThumbnail.isEmpty()) {
        QTemporaryFile tempFile(QDir::tempPath() + QStringLiteral("/kdeconnect_call_XXXXXX.jpg"));
        tempFile.setAutoRemove(false);
        if (tempFile.open()) {
            tempFile.write(phoneThumbnail);
            photoPath = tempFile.fileName();
            tempFile.close();
        }
    }

    const QString cleanName = (contactName.isEmpty() || contactName == i18n("unknown number"))
        ? i18nc("@title", "Panggilan Masuk")
        : contactName;
    const QString cleanNumber = (phoneNumber.isEmpty() || phoneNumber == i18n("unknown number"))
        ? i18nc("@info:status", "Panggilan suara di ponsel Anda")
        : phoneNumber;

    auto dialog = new SendReplyDialog(cleanNumber,
                                      QString(),
                                      device()->name(),
                                      cleanName,
                                      photoPath,
                                      true,
                                      QStringLiteral("answer"),
                                      QStringLiteral("mute"));
    m_currentCallDialog = dialog;

    connect(dialog, &SendReplyDialog::dismissRequested, this, &TelephonyPlugin::sendMutePacket);
    connect(dialog, &SendReplyDialog::actionTriggered, this, [this](const QString &action) {
        if (action == QLatin1String("mute")) {
            sendMutePacket();
            hangupCall();
        } else if (action == QLatin1String("answer")) {
            answerCall();
        } else if (action == QLatin1String("hangup")) {
            hangupCall();
        }
    });

    dialog->show();
    dialog->raise();
}

void TelephonyPlugin::ensureWireplumberConfig()
{
    const QString configPath = QDir::homePath() + QStringLiteral("/.config/wireplumber/wireplumber.conf.d/20-bluez-roles.conf");
    QFile file(configPath);
    if (!file.exists()) {
        QDir().mkpath(QFileInfo(configPath).path());
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            out << "monitor.bluez.properties = {\n"
                << "  bluez5.roles = [ a2dp_sink a2dp_source hfp_hf hfp_ag ]\n"
                << "  bluez5.hfphsp-backend = \"native\"\n"
                << "}\n";
            file.close();
            QProcess::startDetached(QStringLiteral("systemctl"), {QStringLiteral("--user"), QStringLiteral("restart"), QStringLiteral("wireplumber")});
        }
    }
}

void TelephonyPlugin::connectBluetoothDevice()
{
    QDBusMessage msg = QDBusMessage::createMethodCall(
        QStringLiteral("org.bluez"),
        QStringLiteral("/"),
        QStringLiteral("org.freedesktop.DBus.ObjectManager"),
        QStringLiteral("GetManagedObjects")
    );
    QDBusMessage reply = QDBusConnection::systemBus().call(msg);
    if (reply.type() != QDBusMessage::ReplyMessage || reply.arguments().isEmpty()) {
        return;
    }

    const QString devName = device()->name();
    const QDBusArgument arg = reply.arguments().at(0).value<QDBusArgument>();
    arg.beginMap();
    while (!arg.atEnd()) {
        arg.beginMapEntry();
        QDBusObjectPath path;
        arg >> path;
        arg.beginMap();
        while (!arg.atEnd()) {
            arg.beginMapEntry();
            QString iface;
            arg >> iface;
            QVariantMap props;
            arg >> props;
            if (iface == QStringLiteral("org.bluez.Device1")) {
                const QString name = props.value(QStringLiteral("Name")).toString();
                const QString alias = props.value(QStringLiteral("Alias")).toString();
                const QString icon = props.value(QStringLiteral("Icon")).toString();
                const bool paired = props.value(QStringLiteral("Paired")).toBool();
                const bool connected = props.value(QStringLiteral("Connected")).toBool();

                if (paired && !connected && (name == devName || alias == devName || icon == QLatin1String("phone"))) {
                    qCDebug(KDECONNECT_PLUGIN_TELEPHONY) << "Connecting Bluetooth to phone at" << path.path();
                    QDBusInterface btDevice(QStringLiteral("org.bluez"), path.path(), QStringLiteral("org.bluez.Device1"), QDBusConnection::systemBus());
                    btDevice.asyncCall(QStringLiteral("Connect"));
                }
            }
            arg.endMapEntry();
        }
        arg.endMap();
        arg.endMapEntry();
    }
    arg.endMap();
}

QString TelephonyPlugin::findPipewireCallPath(QString *agPathOut)
{
    QString callPath;
    if (agPathOut) {
        agPathOut->clear();
    }

    QDBusMessage msg = QDBusMessage::createMethodCall(
        QStringLiteral("org.pipewire.Telephony"),
        QStringLiteral("/org/pipewire/Telephony"),
        QStringLiteral("org.freedesktop.DBus.ObjectManager"),
        QStringLiteral("GetManagedObjects")
    );
    QDBusMessage reply = QDBusConnection::sessionBus().call(msg);
    if (reply.type() != QDBusMessage::ReplyMessage || reply.arguments().isEmpty()) {
        return callPath;
    }

    const QDBusArgument arg = reply.arguments().at(0).value<QDBusArgument>();
    arg.beginMap();
    while (!arg.atEnd()) {
        arg.beginMapEntry();
        QDBusObjectPath path;
        arg >> path;
        arg.beginMap();
        while (!arg.atEnd()) {
            arg.beginMapEntry();
            QString iface;
            arg >> iface;
            QVariantMap props;
            arg >> props;
            if (iface == QStringLiteral("org.pipewire.Telephony.Call1") || iface == QStringLiteral("org.ofono.VoiceCall")) {
                callPath = path.path();
            } else if (iface == QStringLiteral("org.pipewire.Telephony.AudioGateway1") && agPathOut) {
                *agPathOut = path.path();
            }
            arg.endMapEntry();
        }
        arg.endMap();
        arg.endMapEntry();
    }
    arg.endMap();

    return callPath;
}

void TelephonyPlugin::answerCall()
{
    connectBluetoothDevice();

    QString agPath;
    const QString callPath = findPipewireCallPath(&agPath);

    if (!callPath.isEmpty()) {
        qCDebug(KDECONNECT_PLUGIN_TELEPHONY) << "Answering call via Call1 at" << callPath;
        QDBusInterface callIface(QStringLiteral("org.pipewire.Telephony"), callPath, QStringLiteral("org.pipewire.Telephony.Call1"), QDBusConnection::sessionBus());
        callIface.call(QStringLiteral("Answer"));
    } else if (!agPath.isEmpty()) {
        qCDebug(KDECONNECT_PLUGIN_TELEPHONY) << "Answering call via AudioGateway1 at" << agPath;
        QDBusInterface agIface(QStringLiteral("org.pipewire.Telephony"), agPath, QStringLiteral("org.pipewire.Telephony.AudioGateway1"), QDBusConnection::sessionBus());
        agIface.call(QStringLiteral("ReleaseAndAnswer"));
    } else {
        qCDebug(KDECONNECT_PLUGIN_TELEPHONY) << "No pipewire telephony call found yet, connecting bluetooth";
    }

    if (m_currentCallDialog) {
        m_currentCallDialog->setCallConnected();
    }
}

void TelephonyPlugin::hangupCall()
{
    QString agPath;
    const QString callPath = findPipewireCallPath(&agPath);

    if (!callPath.isEmpty()) {
        qCDebug(KDECONNECT_PLUGIN_TELEPHONY) << "Hanging up call via Call1 at" << callPath;
        QDBusInterface callIface(QStringLiteral("org.pipewire.Telephony"), callPath, QStringLiteral("org.pipewire.Telephony.Call1"), QDBusConnection::sessionBus());
        callIface.call(QStringLiteral("Hangup"));
    } else if (!agPath.isEmpty()) {
        qCDebug(KDECONNECT_PLUGIN_TELEPHONY) << "Hanging up call via AudioGateway1 at" << agPath;
        QDBusInterface agIface(QStringLiteral("org.pipewire.Telephony"), agPath, QStringLiteral("org.pipewire.Telephony.AudioGateway1"), QDBusConnection::sessionBus());
        agIface.call(QStringLiteral("HangupAll"));
    }

    sendMutePacket();

    if (m_currentCallDialog) {
        m_currentCallDialog->close();
        m_currentCallDialog = nullptr;
    }
    if (m_currentCallNotification) {
        m_currentCallNotification->close();
    }
}

void TelephonyPlugin::receivePacket(const NetworkPacket &np)
{
    if (np.get<bool>(QStringLiteral("isCancel"))) {
        if (m_currentCallDialog) {
            m_currentCallDialog->close();
            m_currentCallDialog = nullptr;
        }
        if (m_currentCallNotification) {
            m_currentCallNotification->close();
        }
        return;
    }

    // ignore old style sms packet
    if (np.get<QString>(QStringLiteral("event")) != QLatin1String("sms")) {
        createNotification(np);
    }
}

void TelephonyPlugin::sendMutePacket()
{
    NetworkPacket packet(PACKET_TYPE_TELEPHONY_REQUEST_MUTE, {{QStringLiteral("action"), QStringLiteral("mute")}});
    sendPacket(packet);
}

QString TelephonyPlugin::dbusPath() const
{
    return QLatin1String("/modules/kdeconnect/devices/%1/telephony").arg(device()->id());
}

#include "moc_telephonyplugin.cpp"
#include "telephonyplugin.moc"
