#pragma once

#include <QDateTime>
#include <QObject>

class AuthService;
class QTimer;

// sits in the background while the user is logged in and watches the
// session: logs us out after idleMinutes without any keyboard/mouse
// activity, or when the 7 day absolute limit is hit (whichever comes first)
class SessionWatch : public QObject
{
    Q_OBJECT
public:
    SessionWatch(AuthService& auth, QObject* parent = nullptr);

    void start(qint64 sessionId);
    void stop();

signals:
    void sessionExpired();

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    void tick();

    AuthService& m_auth;
    QTimer* m_timer = nullptr;
    qint64 m_sessionId = 0;
    QDateTime m_lastActivity;
};
