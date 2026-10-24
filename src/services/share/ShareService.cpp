#include "src/services/share/ShareService.hpp"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHostAddress>
#include <QNetworkInterface>
#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTemporaryDir>
#include <QUuid>
#include <QUrl>
#include <QUrlQuery>

#include "src/services/files/FileService.hpp"
#include "src/services/projects/ProjectService.hpp"

namespace {

// the base port and how far to walk up when it's taken
constexpr int basePort = 8787;
constexpr int portTries = 3;

// one past the upload cap so a slightly-oversized request still gets a
// clean 413 instead of a hung connection
constexpr qint64 requestCapBytes = FileService::maxUploadBytes + 64 * 1024;

QString htmlEscape(const QString& text)
{
    QString escaped = text;
    escaped.replace('&', QStringLiteral("&amp;"));
    escaped.replace('<', QStringLiteral("&lt;"));
    escaped.replace('>', QStringLiteral("&gt;"));
    escaped.replace('"', QStringLiteral("&quot;"));
    return escaped;
}

// first non-loopback ipv4 - what a phone on the same network dials
QString lanAddress()
{
    const QList<QNetworkInterface> interfaces = QNetworkInterface::allInterfaces();
    for (const QNetworkInterface& interface : interfaces) {
        if (!(interface.flags() & QNetworkInterface::IsUp)
            || !(interface.flags() & QNetworkInterface::IsRunning)
            || (interface.flags() & QNetworkInterface::IsLoopBack))
            continue;
        const QList<QNetworkAddressEntry> entries = interface.addressEntries();
        for (const QNetworkAddressEntry& entry : entries) {
            if (entry.ip().protocol() == QAbstractSocket::IPv4Protocol)
                return entry.ip().toString();
        }
    }
    return QString();
}

}

// parsed http request for one connection - just enough of a parser for
// a GET page and a multipart POST, nothing more
class ShareService::Request
{
public:
    explicit Request(QTcpSocket* socket)
        : socket(socket)
    {
    }

    // true once the full request (headers + body per content-length)
    // has arrived
    bool feed()
    {
        buffer += socket->readAll();

        if (headerEnd < 0) {
            headerEnd = buffer.indexOf("\r\n\r\n");
            if (headerEnd < 0)
                return false;

            const QByteArray head = buffer.left(headerEnd);
            const QList<QByteArray> lines = head.split('\n');
            if (!lines.isEmpty()) {
                const QList<QByteArray> parts = lines.first().simplified().split(' ');
                if (parts.size() >= 2) {
                    method = QString::fromLatin1(parts.at(0));
                    target = QUrl::fromPercentEncoding(parts.at(1));
                }
            }
            for (int i = 1; i < lines.size(); ++i) {
                const QByteArray line = lines.at(i).trimmed();
                const int colon = line.indexOf(':');
                if (colon <= 0)
                    continue;
                headers.insert(QString::fromLatin1(line.left(colon).toLower()).trimmed(),
                               QString::fromLatin1(line.mid(colon + 1)).trimmed());
            }
        }

        const qint64 want = headers.value(QStringLiteral("content-length")).toLongLong();
        if (want > requestCapBytes) {
            tooBig = true;
            return true;
        }
        return buffer.size() - headerEnd - 4 >= want;
    }

    QByteArray body() const
    {
        const qint64 want = headers.value(QStringLiteral("content-length")).toLongLong();
        return buffer.mid(headerEnd + 4, want);
    }

    QTcpSocket* socket = nullptr;
    QString method;
    QString target;
    QHash<QString, QString> headers;
    QByteArray buffer;
    int headerEnd = -1;
    bool tooBig = false;
};

ShareService& ShareService::instance()
{
    static ShareService service;
    return service;
}

ShareService::ShareService(QObject* parent)
    : QObject(parent)
{
}

bool ShareService::start(qint64 userId)
{
    stop();
    if (userId <= 0)
        return false;

    m_server = new QTcpServer(this);
    for (int attempt = 0; attempt < portTries; ++attempt) {
        if (m_server->listen(QHostAddress::Any, basePort + attempt)) {
            m_port = basePort + attempt;
            break;
        }
    }
    if (!m_server->isListening()) {
        delete m_server;
        m_server = nullptr;
        return false;
    }

    m_userId = userId;
    // fresh token every start - the url is only worth sharing for as
    // long as this session lasts
    m_token = QUuid::createUuid().toString(QUuid::WithoutBraces).left(16);

    connect(m_server, &QTcpServer::newConnection, this, [this]() {
        while (QTcpSocket* socket = m_server->nextPendingConnection()) {
            auto* request = new Request(socket);
            connect(socket, &QTcpSocket::readyRead, this, [this, request]() {
                if (!request->feed())
                    return;
                handle(*request);
                request->socket->disconnectFromHost();
            });
            // covers both the normal close and a phone hanging up
            // mid-request - the request goes with its socket
            connect(socket, &QTcpSocket::disconnected, socket, [request, socket]() {
                delete request;
                socket->deleteLater();
            });
        }
    });
    return true;
}

void ShareService::stop()
{
    if (m_server) {
        m_server->close();
        m_server->deleteLater();
        m_server = nullptr;
    }
    m_token.clear();
    m_port = 0;
    m_userId = 0;
}

bool ShareService::isRunning() const
{
    return m_server && m_server->isListening();
}

QString ShareService::url() const
{
    if (!isRunning())
        return QString();

    const QString host = lanAddress();
    if (host.isEmpty())
        return QString();

    return QStringLiteral("http://%1:%2/t/%3/").arg(host).arg(m_port).arg(m_token);
}

void ShareService::handle(Request& request)
{
    auto respond = [&request](int status, const QByteArray& body,
                              const QByteArray& contentType,
                              const QList<QByteArray>& extraHeaders = {}) {
        QByteArray head = "HTTP/1.1 " + QByteArray::number(status) + " "
            + (status == 200 ? "OK" : status == 303 ? "See Other"
               : status == 404                      ? "Not Found"
               : status == 405                      ? "Method Not Allowed"
               : status == 413                      ? "Payload Too Large"
                                                    : "Bad Request")
            + "\r\nContent-Type: " + contentType
            + "\r\nContent-Length: " + QByteArray::number(body.size())
            + "\r\nConnection: close\r\n";
        for (const QByteArray& header : extraHeaders)
            head += header + "\r\n";
        head += "\r\n";
        request.socket->write(head);
        if (!body.isEmpty())
            request.socket->write(body);
    };

    // every route lives under /t/<token>/ - anything else is a 404 so
    // the server doesn't advertise itself to the network
    const QString prefix = QStringLiteral("/t/%1").arg(m_token);
    if (!request.target.startsWith(prefix)) {
        respond(404, "not found", "text/plain");
        return;
    }
    const QString path = request.target.mid(prefix.size());
    const QString cleanPath = path.startsWith('/') ? path.mid(1) : path;

    if (request.method == QLatin1String("GET")) {
        if (cleanPath.isEmpty() || cleanPath == QLatin1String("index.html")) {
            // the upload redirects back with ?project=N&ok=<name> (or
            // err=...) - pull those off so the banner shows and the
            // project dropdown stays where the phone left it
            const QUrl url(request.target);
            const QUrlQuery query(url);
            const QString ok = query.queryItemValue(QStringLiteral("ok"));
            const QString err = query.queryItemValue(QStringLiteral("err"));
            const qint64 selected = query.queryItemValue(QStringLiteral("project"))
                                        .toLongLong();
            QString message;
            if (!ok.isEmpty())
                message = QStringLiteral("Uploaded %1.").arg(ok);
            else if (!err.isEmpty())
                message = err;
            respond(200, renderPage(selected, message),
                    "text/html; charset=utf-8");
            return;
        }
        respond(404, "not found", "text/plain");
        return;
    }

    if (request.method != QLatin1String("POST")) {
        respond(405, "post only", "text/plain");
        return;
    }
    if (request.tooBig) {
        respond(413, "files are capped at 100 MB", "text/plain");
        return;
    }

    if (cleanPath != QLatin1String("upload")) {
        respond(404, "not found", "text/plain");
        return;
    }

    // multipart/form-data: pull out the project id and the file part
    const QString contentType = request.headers.value(QStringLiteral("content-type"));
    const int boundaryAt = contentType.indexOf(QStringLiteral("boundary="));
    if (boundaryAt < 0) {
        respond(400, "expected multipart form data", "text/plain");
        return;
    }
    QString boundary = contentType.mid(boundaryAt + 9).trimmed();
    if (boundary.startsWith('"') && boundary.endsWith('"'))
        boundary = boundary.mid(1, boundary.size() - 2);

    const QByteArray body = request.body();
    const QByteArray delimiter = "--" + boundary.toLatin1();

    // multipart bodies are delimiter-separated sections; the first
    // chunk before the first delimiter is the preamble (empty here)
    QList<QByteArray> parts;
    int at = 0;
    while (true) {
        const int found = body.indexOf(delimiter, at);
        if (found < 0)
            break;
        parts.append(body.mid(at, found - at));
        at = found + delimiter.size();
        if (body.mid(at, 2) == "--")
            break; // final boundary
    }

    qint64 projectId = 0;
    QString uploadName;
    QByteArray uploadBytes;

    for (QByteArray part : parts) {
        if (part.startsWith("\r\n"))
            part.remove(0, 2);
        if (part.endsWith("\r\n"))
            part.chop(2);
        const int splitAt = part.indexOf("\r\n\r\n");
        if (splitAt < 0)
            continue;
        const QString partHeaders = QString::fromLatin1(part.left(splitAt));
        const QByteArray partBody = part.mid(splitAt + 4);

        if (partHeaders.contains(QStringLiteral("name=\"project\""))) {
            projectId = QString::fromLatin1(partBody).trimmed().toLongLong();
        } else if (partHeaders.contains(QStringLiteral("filename=\""))) {
            const int nameAt = partHeaders.indexOf(QStringLiteral("filename=\""));
            const int nameEnd = partHeaders.indexOf('"', nameAt + 10);
            if (nameEnd > nameAt)
                uploadName = partHeaders.mid(nameAt + 10, nameEnd - nameAt - 10);
            uploadBytes = partBody;
        }
    }

    // only ever keep the file part's base name, whatever the phone sent
    uploadName = QFileInfo(uploadName).fileName();
    if (uploadName.isEmpty() || uploadName == QLatin1String(".")
        || uploadName == QLatin1String("..")) {
        respond(400, "the upload had no filename", "text/plain");
        return;
    }
    if (projectId <= 0) {
        respond(400, "pick a project", "text/plain");
        return;
    }

    // stage under a temp dir with the original name so FileService
    // records what the user actually picked, then clean the dir up
    QTemporaryDir stage;
    if (!stage.isValid()) {
        respond(500, "could not stage the file", "text/plain");
        return;
    }
    const QString stagedPath = stage.filePath(uploadName);
    QFile staged(stagedPath);
    if (!staged.open(QIODevice::WriteOnly) || staged.write(uploadBytes) != uploadBytes.size()) {
        respond(500, "could not stage the file", "text/plain");
        return;
    }
    staged.close();

    FileService files;
    QString error;
    const QString saved = files.upload(stagedPath, projectId, m_userId, &error);

    // let the dashboard's files panel reload without polling
    if (!saved.isEmpty())
        emit fileReceived(projectId);

    QUrl back = request.target;
    back.setPath(QStringLiteral("/t/%1/").arg(m_token));
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("project"), QString::number(projectId));
    if (!saved.isEmpty())
        query.addQueryItem(QStringLiteral("ok"), uploadName);
    else
        query.addQueryItem(QStringLiteral("err"), error);
    back.setQuery(query);

    respond(303, QByteArray(), "text/plain",
            { "Location: " + back.toEncoded() });
}

QByteArray ShareService::renderPage(qint64 selectedProject, const QString& message)
{
    ProjectService projects;
    const QVector<ProjectRef> list = projects.listActive();

    QString options;
    for (const ProjectRef& project : list) {
        options += QStringLiteral(
                       "<option value=\"%1\"%2>%3</option>")
                       .arg(project.id)
                       .arg(project.id == selectedProject
                                ? QStringLiteral(" selected")
                                : QString())
                       .arg(htmlEscape(project.name));
    }

    QString banner;
    if (!message.isEmpty())
        banner = QStringLiteral("<p class=\"banner\">%1</p>").arg(htmlEscape(message));

    // single page, no javascript beyond the form - works from any phone
    // browser, which is the whole point
    const QString html = QStringLiteral(
               "<!doctype html><html><head><meta charset=\"utf-8\">"
               "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"
               "<title>Orbit share</title><style>"
               "body{font-family:system-ui,sans-serif;background:#1b1d23;color:#e8eaf0;"
               "margin:0;padding:24px}"
               "h1{font-size:20px;color:#2dd4bf;margin:0 0 16px}"
               "form{background:#24272f;border:1px solid #323643;border-radius:10px;"
               "padding:16px;display:flex;flex-direction:column;gap:12px;max-width:420px}"
               "select,input[type=file]{font-size:16px;padding:10px;border-radius:8px;"
               "border:1px solid #3a3f4d;background:#1b1d23;color:#e8eaf0;width:100%}"
               "button{font-size:16px;padding:12px;border:none;border-radius:8px;"
               "background:#14b8a6;color:#fff;font-weight:600}"
               ".banner{background:#0f766e;border-radius:8px;padding:10px 14px;"
               "max-width:420px}"
               ".hint{color:#9aa1b2;font-size:13px;max-width:420px}"
               "</style></head><body>"
               "<h1>Orbit share</h1>%1"
               "<p class=\"hint\">Pick a project and a file - it lands in that "
               "project's files, exactly like an upload from the app.</p>"
               "<form method=\"post\" action=\"upload\" enctype=\"multipart/form-data\">"
               "<select name=\"project\">%2</select>"
               "<input type=\"file\" name=\"file\" required>"
               "<button type=\"submit\">Upload</button>"
               "</form></body></html>")
        .arg(banner, options);
    return html.toUtf8();
}
