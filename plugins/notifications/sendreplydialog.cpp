/**
 * SPDX-FileCopyrightText: 2015 Albert Vaca <albertvaka@gmail.com>
 * SPDX-FileCopyrightText: 2026 KDE Connect contributors
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "sendreplydialog.h"

#include <QBoxLayout>
#include <QEnterEvent>
#include <QFile>
#include <QFrame>
#include <QGuiApplication>
#include <QIcon>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include <QScreen>
#include <QTimer>

#include <KLocalizedString>

QPointer<SendReplyDialog> SendReplyDialog::s_currentInstance;

SendReplyDialog::SendReplyDialog(const QString &originalMessage,
                                 const QString &replyId,
                                 const QString &topicName,
                                 const QString &senderName,
                                 const QString &iconPath,
                                 bool isCall,
                                 const QString &answerAction,
                                 const QString &declineAction,
                                 QWidget *parent)
    : QDialog(parent)
{
    initUi(originalMessage, replyId, topicName, senderName, iconPath, isCall, answerAction, declineAction);
}

SendReplyDialog::~SendReplyDialog()
{
    if (s_currentInstance == this) {
        s_currentInstance = nullptr;
    }
}

void SendReplyDialog::initUi(const QString &originalMessage,
                             const QString &replyId,
                             const QString &topicName,
                             const QString &senderName,
                             const QString &iconPath,
                             bool isCall,
                             const QString &answerAction,
                             const QString &declineAction)
{
    if (s_currentInstance && s_currentInstance != this) {
        s_currentInstance->close();
        s_currentInstance = nullptr;
    }
    s_currentInstance = this;

    m_replyId = replyId;
    m_isCall = isCall;
    m_answerAction = answerAction;
    m_declineAction = declineAction;

    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_DeleteOnClose);

    const QString cleanSender = senderName.isEmpty() ? topicName : senderName;
    QString cleanMessage = originalMessage;
    cleanMessage.replace(QStringLiteral("<br/>"), QStringLiteral("\n"));
    cleanMessage.replace(QStringLiteral("<br>"), QStringLiteral("\n"));

    auto mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(16, 16, 16, 16);
    mainLayout->setSpacing(10);

    // =========================================================================
    // CARD 1: Modern Glassmorphic Notification / Call Card
    // =========================================================================
    auto card1 = new QFrame(this);
    card1->setStyleSheet(QStringLiteral(
        "QFrame {"
        "  background-color: #1e1b2e;"
        "  border: 1px solid rgba(255, 255, 255, 0.15);"
        "  border-radius: 16px;"
        "}"
    ));
    auto card1Layout = new QVBoxLayout(card1);
    card1Layout->setContentsMargins(16, 14, 16, 14);
    card1Layout->setSpacing(12);

    // 1.1 Header Row: App Icon + App Name + Chip + Timestamp/Status + Close 'x'
    auto headerLayout = new QHBoxLayout();
    headerLayout->setSpacing(8);

    auto appIconLabel = new QLabel(card1);
    appIconLabel->setFixedSize(22, 22);
    appIconLabel->setStyleSheet(topicName.contains(QLatin1String("WhatsApp"), Qt::CaseInsensitive)
        ? QStringLiteral("background-color: #25D366; border-radius: 6px; border: none;")
        : QStringLiteral("background-color: rgba(255, 255, 255, 0.15); border-radius: 6px; border: none;"));
    appIconLabel->setAlignment(Qt::AlignCenter);
    appIconLabel->setPixmap(QIcon::fromTheme(m_isCall ? QStringLiteral("call-start") : QStringLiteral("kdeconnect")).pixmap(14, 14));
    headerLayout->addWidget(appIconLabel);

    auto appNameLabel = new QLabel(topicName, card1);
    appNameLabel->setStyleSheet(QStringLiteral("color: #ffffff; font-weight: bold; font-size: 11pt; border: none; background: transparent;"));
    headerLayout->addWidget(appNameLabel);

    if (m_isCall) {
        auto callChip = new QLabel(i18nc("@info:status", "Panggilan Suara Masuk"), card1);
        callChip->setStyleSheet(QStringLiteral(
            "color: #4ade80;"
            "background-color: rgba(34, 197, 94, 0.18);"
            "border: 1px solid rgba(34, 197, 94, 0.35);"
            "border-radius: 8px;"
            "padding: 2px 8px;"
            "font-weight: bold;"
            "font-size: 9pt;"
        ));
        headerLayout->addWidget(callChip);
    } else {
        auto viaChip = new QLabel(QStringLiteral("via KDE Connect"), card1);
        viaChip->setStyleSheet(QStringLiteral(
            "color: rgba(255, 255, 255, 0.65);"
            "background-color: rgba(255, 255, 255, 0.08);"
            "border-radius: 8px;"
            "padding: 2px 8px;"
            "font-size: 9pt;"
            "border: none;"
        ));
        headerLayout->addWidget(viaChip);
    }

    headerLayout->addStretch();

    if (m_isCall) {
        m_statusLabel = new QLabel(i18nc("@info:status", "Sedang Berdering..."), card1);
        m_statusLabel->setStyleSheet(QStringLiteral("color: #86efac; font-weight: bold; font-size: 9.5pt; border: none; background: transparent;"));
        headerLayout->addWidget(m_statusLabel);
    } else {
        auto timeLabel = new QLabel(i18nc("@info:status", "Baru saja"), card1);
        timeLabel->setStyleSheet(QStringLiteral("color: rgba(255, 255, 255, 0.5); font-size: 9pt; border: none; background: transparent;"));
        headerLayout->addWidget(timeLabel);
    }

    auto closeBtn = new QPushButton(QStringLiteral("x"), card1);
    closeBtn->setFixedSize(22, 22);
    closeBtn->setStyleSheet(QStringLiteral(
        "QPushButton {"
        "  color: rgba(255, 255, 255, 0.6);"
        "  background: rgba(255, 255, 255, 0.08);"
        "  border: none;"
        "  border-radius: 11px;"
        "  font-weight: bold;"
        "  font-size: 10pt;"
        "}"
        "QPushButton:hover { background: rgba(255, 255, 255, 0.2); color: #ffffff; }"
    ));
    connect(closeBtn, &QPushButton::clicked, this, &QWidget::close);
    headerLayout->addWidget(closeBtn);

    card1Layout->addLayout(headerLayout);

    // 1.2 Body Row: Avatar Circle + Message/Caller Text
    auto bodyLayout = new QHBoxLayout();
    bodyLayout->setSpacing(12);

    auto avatarLabel = new QLabel(card1);
    avatarLabel->setFixedSize(48, 48);
    avatarLabel->setStyleSheet(QStringLiteral("border: none; background: transparent;"));

    QPixmap avatarPix(48, 48);
    avatarPix.fill(Qt::transparent);
    {
        QPainter p(&avatarPix);
        p.setRenderHint(QPainter::Antialiasing);
        QPainterPath clipPath;
        clipPath.addEllipse(0, 0, 48, 48);
        p.setClipPath(clipPath);

        if (!iconPath.isEmpty() && QFile::exists(iconPath)) {
            QPixmap src(iconPath);
            p.drawPixmap(0, 0, 48, 48, src.scaled(48, 48, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
        } else {
            p.fillRect(0, 0, 48, 48, QColor(QStringLiteral("#3c3656")));
            p.setPen(Qt::white);
            p.setFont(QFont(QStringLiteral("sans-serif"), 15, QFont::Bold));
            p.drawText(QRect(0, 0, 48, 48), Qt::AlignCenter, cleanSender.isEmpty() ? QStringLiteral("K") : cleanSender.left(1).toUpper());
        }

        p.setClipping(false);
        p.setBrush(QColor(QStringLiteral("#22c55e")));
        p.setPen(QPen(QColor(QStringLiteral("#201d33")), 2));
        p.drawEllipse(34, 34, 12, 12);
    }
    avatarLabel->setPixmap(avatarPix);
    bodyLayout->addWidget(avatarLabel, 0, Qt::AlignTop);

    auto textLayout = new QVBoxLayout();
    textLayout->setSpacing(2);

    auto senderLabel = new QLabel(cleanSender, card1);
    senderLabel->setStyleSheet(QStringLiteral("color: #ffffff; font-weight: bold; font-size: 13pt; border: none; background: transparent;"));
    textLayout->addWidget(senderLabel);

    const QString displayMessage = m_isCall
        ? (cleanMessage.isEmpty() ? i18nc("@info:status", "Panggilan suara masuk") : cleanMessage)
        : cleanMessage;
    auto messageLabel = new QLabel(displayMessage, card1);
    messageLabel->setStyleSheet(QStringLiteral("color: #cbd5e1; font-size: 10pt; border: none; background: transparent;"));
    messageLabel->setWordWrap(true);
    textLayout->addWidget(messageLabel);

    bodyLayout->addLayout(textLayout, 1);
    card1Layout->addLayout(bodyLayout);

    // 1.3 Actions Row: WhatsApp Call actions OR Notification quick actions
    auto actionLayout = new QHBoxLayout();
    actionLayout->setSpacing(8);

    if (m_isCall) {
        // Red Tolak button
        auto declineBtn = new QPushButton(QIcon::fromTheme(QStringLiteral("call-stop")), i18nc("@action:button", "Tolak"), card1);
        declineBtn->setStyleSheet(QStringLiteral(
            "QPushButton {"
            "  color: #ffffff;"
            "  background-color: #dc2626;"
            "  border: none;"
            "  border-radius: 12px;"
            "  padding: 8px 24px;"
            "  font-weight: bold;"
            "  font-size: 10pt;"
            "}"
            "QPushButton:hover { background-color: #ef4444; }"
            "QPushButton:pressed { background-color: #b91c1c; }"
        ));
        connect(declineBtn, &QPushButton::clicked, this, [this]() {
            if (!m_declineAction.isEmpty()) {
                Q_EMIT actionTriggered(m_declineAction);
            } else {
                Q_EMIT dismissRequested();
            }
            close();
        });
        actionLayout->addWidget(declineBtn);

        actionLayout->addStretch();

        // Green Angkat button
        auto answerBtn = new QPushButton(QIcon::fromTheme(QStringLiteral("call-start")), i18nc("@action:button", "Angkat"), card1);
        answerBtn->setStyleSheet(QStringLiteral(
            "QPushButton {"
            "  color: #ffffff;"
            "  background-color: #16a34a;"
            "  border: none;"
            "  border-radius: 12px;"
            "  padding: 8px 30px;"
            "  font-weight: bold;"
            "  font-size: 10.5pt;"
            "}"
            "QPushButton:hover { background-color: #22c55e; }"
            "QPushButton:pressed { background-color: #15803d; }"
        ));
        connect(answerBtn, &QPushButton::clicked, this, [this, answerBtn, declineBtn, messageLabel]() {
            if (!m_answerAction.isEmpty()) {
                Q_EMIT actionTriggered(m_answerAction);
            }
            if (m_statusLabel) {
                m_statusLabel->setText(i18nc("@info:status", "Bicara di Ponsel"));
                m_statusLabel->setStyleSheet(QStringLiteral("color: #4ade80; font-weight: bold; font-size: 9.5pt; border: none; background: transparent;"));
            }
            if (messageLabel) {
                messageLabel->setText(i18nc("@info:status", "Panggilan aktif di HP. Hubungkan Bluetooth jika ingin bicara lewat PC"));
                messageLabel->setStyleSheet(QStringLiteral("color: #cbd5e1; font-size: 9.5pt; border: none; background: transparent;"));
            }
            answerBtn->setEnabled(false);
            declineBtn->setEnabled(false);
            QTimer::singleShot(4000, this, &QWidget::close);
        });
        actionLayout->addWidget(answerBtn);
    } else {
        auto markReadBtn = new QPushButton(i18nc("@action:button", "Tandai sudah dibaca"), card1);
        markReadBtn->setStyleSheet(QStringLiteral(
            "QPushButton {"
            "  color: #e2e8f0;"
            "  background: rgba(255, 255, 255, 0.08);"
            "  border: 1px solid rgba(255, 255, 255, 0.1);"
            "  border-radius: 12px;"
            "  padding: 6px 14px;"
            "  font-size: 9.5pt;"
            "}"
            "QPushButton:hover { background: rgba(255, 255, 255, 0.14); }"
        ));
        connect(markReadBtn, &QPushButton::clicked, this, [this]() {
            Q_EMIT dismissRequested();
            close();
        });
        actionLayout->addWidget(markReadBtn);

        auto muteBtn = new QPushButton(i18nc("@action:button", "Senyapkan"), card1);
        muteBtn->setStyleSheet(QStringLiteral(
            "QPushButton {"
            "  color: #e2e8f0;"
            "  background: rgba(255, 255, 255, 0.08);"
            "  border: 1px solid rgba(255, 255, 255, 0.1);"
            "  border-radius: 12px;"
            "  padding: 6px 14px;"
            "  font-size: 9.5pt;"
            "}"
            "QPushButton:hover { background: rgba(255, 255, 255, 0.14); }"
        ));
        connect(muteBtn, &QPushButton::clicked, this, &QWidget::close);
        actionLayout->addWidget(muteBtn);

        actionLayout->addStretch();

        if (!m_replyId.isEmpty()) {
            auto replyBtn = new QPushButton(i18nc("@action:button", "Balas"), card1);
            replyBtn->setStyleSheet(QStringLiteral(
                "QPushButton {"
                "  color: #ffffff;"
                "  background: #2563eb;"
                "  border: none;"
                "  border-radius: 12px;"
                "  padding: 6px 20px;"
                "  font-weight: bold;"
                "  font-size: 10pt;"
                "}"
                "QPushButton:hover { background: #3b82f6; }"
                "QPushButton:pressed { background: #1d4ed8; }"
            ));
            connect(replyBtn, &QPushButton::clicked, this, [this]() {
                if (m_replyInput) {
                    m_replyInput->setFocus();
                }
            });
            actionLayout->addWidget(replyBtn);
        }
    }

    card1Layout->addLayout(actionLayout);
    mainLayout->addWidget(card1);

    // =========================================================================
    // CARD 2: Quick Reply Card (Directly underneath Card 1)
    // =========================================================================
    if (!m_replyId.isEmpty() && !m_isCall) {
        auto card2 = new QFrame(this);
        card2->setStyleSheet(QStringLiteral(
            "QFrame {"
            "  background-color: #191627;"
            "  border: 1px solid rgba(255, 255, 255, 0.12);"
            "  border-radius: 16px;"
            "}"
        ));
        auto card2Layout = new QVBoxLayout(card2);
        card2Layout->setContentsMargins(16, 12, 16, 12);
        card2Layout->setSpacing(10);

        auto replyHeaderLayout = new QHBoxLayout();
        replyHeaderLayout->setSpacing(6);

        auto blueDot = new QLabel(QStringLiteral("*"), card2);
        blueDot->setStyleSheet(QStringLiteral("color: #3b82f6; font-size: 12pt; font-weight: bold; border: none; background: transparent;"));
        replyHeaderLayout->addWidget(blueDot);

        auto replyHeaderLabel = new QLabel(i18nc("@title", "Balasan Cepat ke %1", cleanSender), card2);
        replyHeaderLabel->setStyleSheet(QStringLiteral("color: #ffffff; font-weight: bold; font-size: 10.5pt; border: none; background: transparent;"));
        replyHeaderLayout->addWidget(replyHeaderLabel);

        replyHeaderLayout->addStretch();

        auto replyAppLabel = new QLabel(topicName, card2);
        replyAppLabel->setStyleSheet(QStringLiteral("color: rgba(255, 255, 255, 0.4); font-size: 9pt; border: none; background: transparent;"));
        replyHeaderLayout->addWidget(replyAppLabel);

        card2Layout->addLayout(replyHeaderLayout);

        auto inputRowLayout = new QHBoxLayout();
        inputRowLayout->setSpacing(8);

        m_replyInput = new QLineEdit(card2);
        m_replyInput->setPlaceholderText(i18nc("@info:placeholder", "Iya, ada apa?"));
        m_replyInput->setStyleSheet(QStringLiteral(
            "QLineEdit {"
            "  background-color: rgba(255, 255, 255, 0.06);"
            "  color: #ffffff;"
            "  border: 1px solid rgba(255, 255, 255, 0.12);"
            "  border-radius: 12px;"
            "  padding: 8px 12px;"
            "  font-size: 10pt;"
            "}"
            "QLineEdit:focus {"
            "  border: 1.5px solid #3b82f6;"
            "}"
        ));
        inputRowLayout->addWidget(m_replyInput, 1);

        auto sendBtn = new QPushButton(i18nc("@action:button", "Kirim"), card2);
        sendBtn->setStyleSheet(QStringLiteral(
            "QPushButton {"
            "  background-color: #2563eb;"
            "  color: #ffffff;"
            "  font-weight: bold;"
            "  border: none;"
            "  border-radius: 10px;"
            "  padding: 8px 22px;"
            "  font-size: 10pt;"
            "}"
            "QPushButton:hover { background-color: #3b82f6; }"
            "QPushButton:pressed { background-color: #1d4ed8; }"
        ));
        const auto onSend = [this]() {
            const QString text = m_replyInput->text().trimmed();
            if (!text.isEmpty()) {
                Q_EMIT sendReply(m_replyId, text);
            }
            close();
        };
        connect(sendBtn, &QPushButton::clicked, this, onSend);
        connect(m_replyInput, &QLineEdit::returnPressed, this, onSend);
        inputRowLayout->addWidget(sendBtn);

        card2Layout->addLayout(inputRowLayout);
        mainLayout->addWidget(card2);

        connect(m_replyInput, &QLineEdit::textChanged, this, [this](const QString &text) {
            if (!text.isEmpty() && m_autoCloseTimer) {
                m_autoCloseTimer->stop();
            }
        });
    }

    // Auto dismiss timer for regular notifications (8 seconds)
    if (!m_isCall) {
        m_autoCloseTimer = new QTimer(this);
        m_autoCloseTimer->setSingleShot(true);
        connect(m_autoCloseTimer, &QTimer::timeout, this, &QWidget::close);
        m_autoCloseTimer->start(8000);
    }

    positionUnderClock();
    if (m_replyInput) {
        m_replyInput->setFocus();
    }
}

void SendReplyDialog::positionUnderClock()
{
    setFixedWidth(520);
    adjustSize();

    QScreen *screen = QGuiApplication::primaryScreen();
    if (screen) {
        QRect geom = screen->geometry();
        int targetX = geom.x() + (geom.width() - 520) / 2;
        int targetY = geom.y() + 38;
        move(targetX, targetY);
    }
}

void SendReplyDialog::showEvent(QShowEvent *event)
{
    QDialog::showEvent(event);
    positionUnderClock();
}

void SendReplyDialog::enterEvent(QEnterEvent *event)
{
    if (m_autoCloseTimer && m_autoCloseTimer->isActive()) {
        m_autoCloseTimer->stop();
    }
    QDialog::enterEvent(event);
}

void SendReplyDialog::leaveEvent(QEvent *event)
{
    if (m_autoCloseTimer && !m_isCall) {
        if (!m_replyInput || m_replyInput->text().trimmed().isEmpty()) {
            m_autoCloseTimer->start(4000);
        }
    }
    QDialog::leaveEvent(event);
}

QSize SendReplyDialog::sizeHint() const
{
    return QSize(520, m_isCall ? 170 : 250);
}

void SendReplyDialog::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        close();
        event->accept();
        return;
    }
    QDialog::keyPressEvent(event);
}

#include "moc_sendreplydialog.cpp"
