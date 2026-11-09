/**
 * SPDX-FileCopyrightText: 2015 Albert Vaca <albertvaka@gmail.com>
 * SPDX-FileCopyrightText: 2026 KDE Connect contributors
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "sendreplydialog.h"

#include <QBoxLayout>
#include <QCloseEvent>
#include <QCursor>
#include <QEasingCurve>
#include <QEnterEvent>
#include <QFile>
#include <QFrame>
#include <QGraphicsDropShadowEffect>
#include <QGuiApplication>
#include <QIcon>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QPainter>
#include <QPainterPath>
#include <QParallelAnimationGroup>
#include <QPropertyAnimation>
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
                                 const QStringList &actions,
                                 QWidget *parent)
    : QDialog(parent)
{
    initUi(originalMessage, replyId, topicName, senderName, iconPath, isCall, answerAction, declineAction, actions);
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
                             const QString &declineAction,
                             const QStringList &actions)
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
    m_actions = actions;

    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_DeleteOnClose);

    const QString cleanSender = senderName.isEmpty() ? topicName : senderName;
    QString cleanMessage = originalMessage;
    cleanMessage.replace(QStringLiteral("<br/>"), QStringLiteral("\n"));
    cleanMessage.replace(QStringLiteral("<br>"), QStringLiteral("\n"));

    auto mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(16, 16, 16, 16);
    mainLayout->setSpacing(0);

    // =========================================================================
    // UNIFIED LUXURY GLASSMORPHIC NOTIFICATION & REPLY CARD
    // =========================================================================
    auto card = new QFrame(this);
    card->setObjectName(QStringLiteral("mainCard"));
    card->setStyleSheet(QStringLiteral(
        "QFrame#mainCard {"
        "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #1e1b30, stop:1 #131120);"
        "  border: 1px solid rgba(255, 255, 255, 0.14);"
        "  border-radius: 20px;"
        "}"
    ));

    auto shadow = new QGraphicsDropShadowEffect(card);
    shadow->setBlurRadius(28);
    shadow->setColor(QColor(0, 0, 0, 160));
    shadow->setOffset(0, 8);
    card->setGraphicsEffect(shadow);

    auto cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(18, 14, 18, 16);
    cardLayout->setSpacing(12);

    // 1. Header Row: App Icon + App Name + Chip + Timestamp/Status + Close 'x'
    auto headerLayout = new QHBoxLayout();
    headerLayout->setSpacing(8);

    auto appIconLabel = new QLabel(card);
    appIconLabel->setFixedSize(22, 22);
    appIconLabel->setStyleSheet(topicName.contains(QLatin1String("WhatsApp"), Qt::CaseInsensitive)
        ? QStringLiteral("background-color: #25D366; border-radius: 6px; border: none;")
        : QStringLiteral("background-color: rgba(255, 255, 255, 0.15); border-radius: 6px; border: none;"));
    appIconLabel->setAlignment(Qt::AlignCenter);
    appIconLabel->setPixmap(QIcon::fromTheme(m_isCall ? QStringLiteral("call-start") : QStringLiteral("kdeconnect")).pixmap(14, 14));
    headerLayout->addWidget(appIconLabel);

    auto appNameLabel = new QLabel(topicName, card);
    appNameLabel->setStyleSheet(QStringLiteral("color: #ffffff; font-weight: bold; font-size: 11pt; border: none; background: transparent;"));
    headerLayout->addWidget(appNameLabel);

    if (m_isCall) {
        auto callChip = new QLabel(i18nc("@info:status", "Panggilan Suara Masuk"), card);
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
        auto viaChip = new QLabel(QStringLiteral("via KDE Connect"), card);
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
        m_statusLabel = new QLabel(i18nc("@info:status", "Sedang Berdering..."), card);
        m_statusLabel->setStyleSheet(QStringLiteral("color: #86efac; font-weight: bold; font-size: 9.5pt; border: none; background: transparent;"));
        headerLayout->addWidget(m_statusLabel);
    } else {
        auto timeLabel = new QLabel(i18nc("@info:status", "Baru saja"), card);
        timeLabel->setStyleSheet(QStringLiteral("color: rgba(255, 255, 255, 0.5); font-size: 9pt; border: none; background: transparent;"));
        headerLayout->addWidget(timeLabel);
    }

    auto closeBtn = new QPushButton(QStringLiteral("x"), card);
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
    connect(closeBtn, &QPushButton::clicked, this, &SendReplyDialog::animateClose);
    headerLayout->addWidget(closeBtn);

    cardLayout->addLayout(headerLayout);

    // 2. Body Row: Avatar Circle + Message/Caller Text
    auto bodyLayout = new QHBoxLayout();
    bodyLayout->setSpacing(12);

    auto avatarLabel = new QLabel(card);
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

    auto senderLabel = new QLabel(cleanSender, card);
    senderLabel->setStyleSheet(QStringLiteral("color: #ffffff; font-weight: bold; font-size: 13pt; border: none; background: transparent;"));
    textLayout->addWidget(senderLabel);

    const QString displayMessage = m_isCall
        ? (cleanMessage.isEmpty() ? i18nc("@info:status", "Panggilan suara masuk") : cleanMessage)
        : cleanMessage;
    m_messageLabel = new QLabel(displayMessage, card);
    m_messageLabel->setStyleSheet(QStringLiteral("color: #cbd5e1; font-size: 10pt; border: none; background: transparent;"));
    m_messageLabel->setWordWrap(true);
    textLayout->addWidget(m_messageLabel);

    bodyLayout->addLayout(textLayout, 1);
    cardLayout->addLayout(bodyLayout);

    // 3. Actions Row
    auto actionLayout = new QHBoxLayout();
    actionLayout->setSpacing(8);

    if (m_isCall) {
        m_declineBtn = new QPushButton(QIcon::fromTheme(QStringLiteral("call-stop")), i18nc("@action:button", "Tolak"), card);
        m_declineBtn->setStyleSheet(QStringLiteral(
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
        connect(m_declineBtn, &QPushButton::clicked, this, [this]() {
            if (!m_declineAction.isEmpty()) {
                Q_EMIT actionTriggered(m_declineAction);
            } else {
                Q_EMIT dismissRequested();
            }
            animateClose();
        });
        actionLayout->addWidget(m_declineBtn);
        actionLayout->addStretch();

        m_answerBtn = new QPushButton(QIcon::fromTheme(QStringLiteral("call-start")), i18nc("@action:button", "Angkat"), card);
        m_answerBtn->setStyleSheet(QStringLiteral(
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
        connect(m_answerBtn, &QPushButton::clicked, this, [this]() {
            if (!m_answerAction.isEmpty()) {
                Q_EMIT actionTriggered(m_answerAction);
            }
            setCallConnected();
        });
        actionLayout->addWidget(m_answerBtn);
        cardLayout->addLayout(actionLayout);
    } else {
        auto markReadBtn = new QPushButton(i18nc("@action:button", "Tandai sudah dibaca"), card);
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
            animateClose();
        });
        actionLayout->addWidget(markReadBtn);

        auto muteBtn = new QPushButton(i18nc("@action:button", "Senyapkan"), card);
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
        connect(muteBtn, &QPushButton::clicked, this, &SendReplyDialog::animateClose);
        actionLayout->addWidget(muteBtn);

        for (const QString &action : std::as_const(m_actions)) {
            const QString lowerAction = action.toLower();
            if (lowerAction.contains(QLatin1String("reply")) || lowerAction.contains(QLatin1String("balas"))) {
                continue;
            }
            auto actionBtn = new QPushButton(action, card);
            actionBtn->setStyleSheet(QStringLiteral(
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
            connect(actionBtn, &QPushButton::clicked, this, [this, action]() {
                Q_EMIT actionTriggered(action);
                animateClose();
            });
            actionLayout->addWidget(actionBtn);
        }

        actionLayout->addStretch();
        cardLayout->addLayout(actionLayout);
    }

    // 4. Quick Reply Section (Integrated seamlessly in the same card)
    if (!m_replyId.isEmpty() && !m_isCall) {
        auto divider = new QFrame(card);
        divider->setFixedHeight(1);
        divider->setStyleSheet(QStringLiteral("background-color: rgba(255, 255, 255, 0.08); border: none;"));
        cardLayout->addWidget(divider);

        auto replyRowLayout = new QHBoxLayout();
        replyRowLayout->setSpacing(8);

        m_replyInput = new QLineEdit(card);
        m_replyInput->setPlaceholderText(i18nc("@info:placeholder", "Iya, ada apa?"));
        m_replyInput->setStyleSheet(QStringLiteral(
            "QLineEdit {"
            "  background-color: rgba(255, 255, 255, 0.06);"
            "  color: #ffffff;"
            "  border: 1px solid rgba(255, 255, 255, 0.12);"
            "  border-radius: 12px;"
            "  padding: 8px 14px;"
            "  font-size: 10pt;"
            "}"
            "QLineEdit:focus {"
            "  background-color: rgba(255, 255, 255, 0.09);"
            "  border: 1.5px solid #3b82f6;"
            "}"
        ));
        replyRowLayout->addWidget(m_replyInput, 1);

        auto sendBtn = new QPushButton(i18nc("@action:button", "Kirim"), card);
        sendBtn->setStyleSheet(QStringLiteral(
            "QPushButton {"
            "  background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #2563eb, stop:1 #3b82f6);"
            "  color: #ffffff;"
            "  font-weight: bold;"
            "  border: none;"
            "  border-radius: 12px;"
            "  padding: 8px 22px;"
            "  font-size: 10pt;"
            "}"
            "QPushButton:hover { background: #3b82f6; }"
            "QPushButton:pressed { background: #1d4ed8; }"
        ));

        const auto onSend = [this, sendBtn]() {
            const QString text = m_replyInput->text().trimmed();
            if (!text.isEmpty()) {
                Q_EMIT sendReply(m_replyId, text);
                sendBtn->setText(QStringLiteral("OK"));
                sendBtn->setStyleSheet(QStringLiteral(
                    "QPushButton {"
                    "  background-color: #16a34a;"
                    "  color: #ffffff;"
                    "  font-weight: bold;"
                    "  border: none;"
                    "  border-radius: 12px;"
                    "  padding: 8px 18px;"
                    "  font-size: 10pt;"
                    "}"
                ));
            }
            animateClose();
        };

        connect(sendBtn, &QPushButton::clicked, this, onSend);
        connect(m_replyInput, &QLineEdit::returnPressed, this, onSend);
        replyRowLayout->addWidget(sendBtn);

        cardLayout->addLayout(replyRowLayout);

        connect(m_replyInput, &QLineEdit::textChanged, this, [this](const QString &text) {
            if (!text.isEmpty() && m_autoCloseTimer) {
                m_autoCloseTimer->stop();
            }
        });
    }

    mainLayout->addWidget(card);

    // Auto dismiss timer for regular notifications (8 seconds)
    if (!m_isCall) {
        m_autoCloseTimer = new QTimer(this);
        m_autoCloseTimer->setSingleShot(true);
        connect(m_autoCloseTimer, &QTimer::timeout, this, &SendReplyDialog::animateClose);
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

    QScreen *screen = QGuiApplication::screenAt(QCursor::pos());
    if (!screen) {
        screen = QGuiApplication::primaryScreen();
    }
    if (screen) {
        const QRect geom = screen->geometry();
        const int targetX = geom.x() + (geom.width() - width()) / 2;
        const int targetY = geom.y() + 38;
        move(targetX, targetY);
    }
}

void SendReplyDialog::animateShow()
{
    if (m_isClosing) {
        return;
    }

    setFixedWidth(520);
    adjustSize();

    QScreen *screen = QGuiApplication::screenAt(QCursor::pos());
    if (!screen) {
        screen = QGuiApplication::primaryScreen();
    }
    if (!screen) {
        return;
    }

    const QRect geom = screen->geometry();
    const int targetX = geom.x() + (geom.width() - width()) / 2;
    const int targetY = geom.y() + 38;
    const QPoint endPos(targetX, targetY);
    const QPoint startPos(targetX, targetY - 45);

    move(startPos);
    setWindowOpacity(0.0);

    auto animPos = new QPropertyAnimation(this, "pos", this);
    animPos->setDuration(320);
    animPos->setStartValue(startPos);
    animPos->setEndValue(endPos);
    animPos->setEasingCurve(QEasingCurve::OutBack);

    auto animOpacity = new QPropertyAnimation(this, "windowOpacity", this);
    animOpacity->setDuration(260);
    animOpacity->setStartValue(0.0);
    animOpacity->setEndValue(1.0);

    auto group = new QParallelAnimationGroup(this);
    group->addAnimation(animPos);
    group->addAnimation(animOpacity);
    group->start(QAbstractAnimation::DeleteWhenStopped);
}

void SendReplyDialog::animateClose()
{
    if (m_isClosing) {
        return;
    }
    m_isClosing = true;

    if (m_autoCloseTimer) {
        m_autoCloseTimer->stop();
    }
    if (m_callDurationTimer) {
        m_callDurationTimer->stop();
    }

    const QPoint curPos = pos();
    const QPoint endPos(curPos.x(), curPos.y() - 35);

    auto animPos = new QPropertyAnimation(this, "pos", this);
    animPos->setDuration(220);
    animPos->setStartValue(curPos);
    animPos->setEndValue(endPos);
    animPos->setEasingCurve(QEasingCurve::InCubic);

    auto animOpacity = new QPropertyAnimation(this, "windowOpacity", this);
    animOpacity->setDuration(200);
    animOpacity->setStartValue(windowOpacity());
    animOpacity->setEndValue(0.0);

    auto group = new QParallelAnimationGroup(this);
    group->addAnimation(animPos);
    group->addAnimation(animOpacity);
    connect(group, &QParallelAnimationGroup::finished, this, [this]() {
        QDialog::close();
    });
    group->start(QAbstractAnimation::DeleteWhenStopped);
}

void SendReplyDialog::showEvent(QShowEvent *event)
{
    QDialog::showEvent(event);
    animateShow();
}

void SendReplyDialog::closeEvent(QCloseEvent *event)
{
    if (!m_isClosing) {
        event->ignore();
        animateClose();
    } else {
        event->accept();
    }
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
    if (m_isCall) {
        return QSize(520, 160);
    }
    if (m_replyId.isEmpty()) {
        return QSize(520, 130);
    }
    return QSize(520, 190);
}

void SendReplyDialog::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        animateClose();
        event->accept();
        return;
    }
    QDialog::keyPressEvent(event);
}

void SendReplyDialog::setCallConnected()
{
    if (m_autoCloseTimer && m_autoCloseTimer->isActive()) {
        m_autoCloseTimer->stop();
    }

    if (m_statusLabel) {
        m_statusLabel->setText(i18nc("@info:status", "Panggilan Terhubung"));
        m_statusLabel->setStyleSheet(QStringLiteral("color: #4ade80; font-weight: bold; font-size: 9.5pt; border: none; background: transparent;"));
    }

    if (m_messageLabel) {
        m_messageLabel->setText(i18nc("@info:status", "Audio Laptop Aktif (Mic & Speaker Terhubung)"));
        m_messageLabel->setStyleSheet(QStringLiteral("color: #86efac; font-size: 9.5pt; border: none; background: transparent;"));
    }

    if (m_answerBtn) {
        m_answerBtn->hide();
    }

    if (m_declineBtn) {
        m_declineBtn->disconnect();
        m_declineBtn->setText(i18nc("@action:button", "Tutup Panggilan"));
        m_declineBtn->setIcon(QIcon::fromTheme(QStringLiteral("call-stop")));
        m_declineBtn->setStyleSheet(QStringLiteral(
            "QPushButton {"
            "  color: #ffffff;"
            "  background-color: #dc2626;"
            "  border: none;"
            "  border-radius: 12px;"
            "  padding: 8px 30px;"
            "  font-weight: bold;"
            "  font-size: 10pt;"
            "}"
            "QPushButton:hover { background-color: #ef4444; }"
            "QPushButton:pressed { background-color: #b91c1c; }"
        ));
        connect(m_declineBtn, &QPushButton::clicked, this, [this]() {
            Q_EMIT actionTriggered(QStringLiteral("hangup"));
            close();
        });
    }

    if (!m_callDurationTimer) {
        m_callDurationTimer = new QTimer(this);
        m_callDurationSeconds = 0;
        connect(m_callDurationTimer, &QTimer::timeout, this, [this]() {
            m_callDurationSeconds++;
            const int mins = m_callDurationSeconds / 60;
            const int secs = m_callDurationSeconds % 60;
            if (m_statusLabel) {
                m_statusLabel->setText(QStringLiteral("Terhubung (%1:%2)")
                    .arg(mins, 2, 10, QLatin1Char('0'))
                    .arg(secs, 2, 10, QLatin1Char('0')));
            }
        });
        m_callDurationTimer->start(1000);
    }
}

#include "moc_sendreplydialog.cpp"
