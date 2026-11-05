/**
 * SPDX-FileCopyrightText: 2015 Albert Vaca <albertvaka@gmail.com>
 * SPDX-FileCopyrightText: 2026 KDE Connect contributors
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <QDialog>
#include <QPointer>
#include <QSize>

class QLineEdit;
class QLabel;
class QPushButton;
class QTimer;
class QEnterEvent;

class SendReplyDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SendReplyDialog(const QString &originalMessage,
                             const QString &replyId,
                             const QString &topicName,
                             const QString &senderName = QString(),
                             const QString &iconPath = QString(),
                             bool isCall = false,
                             const QString &answerAction = QString(),
                             const QString &declineAction = QString(),
                             QWidget *parent = nullptr);
    ~SendReplyDialog() override;

    QSize sizeHint() const override;

public Q_SLOTS:
    void setCallConnected();

Q_SIGNALS:
    void sendReply(const QString &replyId, const QString &messageBody);
    void dismissRequested();
    void actionTriggered(const QString &action);

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void showEvent(QShowEvent *event) override;

private:
    void initUi(const QString &originalMessage,
                const QString &replyId,
                const QString &topicName,
                const QString &senderName,
                const QString &iconPath,
                bool isCall,
                const QString &answerAction,
                const QString &declineAction);
    void positionUnderClock();

    QString m_replyId;
    QString m_answerAction;
    QString m_declineAction;
    bool m_isCall = false;

    QLineEdit *m_replyInput = nullptr;
    QLabel *m_statusLabel = nullptr;
    QLabel *m_messageLabel = nullptr;
    QPushButton *m_answerBtn = nullptr;
    QPushButton *m_declineBtn = nullptr;
    QTimer *m_autoCloseTimer = nullptr;
    QTimer *m_callDurationTimer = nullptr;
    int m_callDurationSeconds = 0;

    static QPointer<SendReplyDialog> s_currentInstance;
};
