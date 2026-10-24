#pragma once

#include <QObject>
#include <QString>

class QTcpServer;

// day 12: local share (phone -> pc). a tiny http server on the lan that
// serves a one-page upload form. the url carries a random token so a
// stray device on the network can't just stumble on it. uploads land
// through FileService exactly like a desktop upload, attributed to the
// signed-in user. started from settings, stopped on logout
class ShareService : public QObject
{
    Q_OBJECT
public:
    static ShareService& instance();

    // binds a port and mints a fresh token; false if no port was free
    bool start(qint64 userId);
    void stop();
    bool isRunning() const;

    // http://<lan-ip>:<port>/t/<token>/ - empty when not running
    QString url() const;

signals:
    // fired after a phone upload lands so the dashboard's files panel
    // can reload without polling
    void fileReceived(qint64 projectId);

private:
    explicit ShareService(QObject* parent = nullptr);

    class Request;
    void handle(Request& request);
    QByteArray renderPage(qint64 selectedProject, const QString& message);

    QTcpServer* m_server = nullptr;
    qint64 m_userId = 0;
    QString m_token;
    int m_port = 0;
};
