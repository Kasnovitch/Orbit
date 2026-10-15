#include "src/data/auth/SessionWatch.hpp"

#include <QCoreApplication>
#include <QEvent>
#include <QTimer>

#include "src/data/auth/AuthService.hpp"

SessionWatch::SessionWatch(AuthService& auth, QObject* parent)
    : QObject(parent)
    , m_auth(auth)
{
    m_timer = new QTimer(this);
    m_timer->setInterval(60 * 1000); // check once a minute
    connect(m_timer, &QTimer::timeout, this, &SessionWatch::tick);

    if (QCoreApplication::instance())
        QCoreApplication::instance()->installEventFilter(this);
}

void SessionWatch::start(qint64 sessionId)
{
    m_sessionId = sessionId;
    m_lastActivity = QDateTime::currentDateTime();
    m_timer->start();
}

void SessionWatch::stop()
{
    m_timer->stop();
    m_sessionId = 0;
}

bool SessionWatch::eventFilter(QObject* watched, QEvent* event)
{
    if (m_sessionId > 0) {
        switch (event->type()) {
        case QEvent::KeyPress:
        case QEvent::MouseButtonPress:
        case QEvent::MouseButtonDblClick:
        case QEvent::MouseMove:
        case QEvent::Wheel:
        case QEvent::TouchBegin:
            m_lastActivity = QDateTime::currentDateTime();
            break;
        default:
            break;
        }
    }
    return QObject::eventFilter(watched, event);
}

void SessionWatch::tick()
{
    if (m_sessionId <= 0)
        return;

    // been away too long in this app?
    const qint64 idleFor = m_lastActivity.secsTo(QDateTime::currentDateTime());
    if (idleFor >= AuthService::idleMinutes * 60) {
        emit sessionExpired();
        return;
    }

    // row revoked from another place, or past the absolute limit?
    if (!m_auth.slideSession(m_sessionId))
        emit sessionExpired();
}
