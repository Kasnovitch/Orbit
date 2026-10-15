#include "src/data/auth/AuthService.hpp"

#include <QCryptographicHash>
#include <QDateTime>
#include <QPasswordDigestor>
#include <QRandomGenerator>
#include <QSettings>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

namespace {
const int pbkdf2Iterations = 100000;
const char* hashPrefix = "pbkdf2-sha256";
}

bool AuthService::seedDefaultUser()
{
    QSqlQuery count;
    if (!count.exec("select count(*) from users") || !count.next())
        return false;
    if (count.value(0).toLongLong() > 0)
        return false;

    QSqlQuery insert;
    insert.prepare(
        "insert into users (username, password_hash, role, display_name) "
        "values (?, ?, ?, ?)");
    insert.addBindValue("admin");
    insert.addBindValue(hashPassword("admin123"));
    insert.addBindValue("admin");
    insert.addBindValue("Administrator");

    if (!insert.exec())
        return false;

    return true;
}

bool AuthService::registerUser(const QString& username, const QString& password,
                               const QString& confirmPassword, QString* error)
{
    const QString name = username.trimmed();

    if (name.isEmpty()) {
        if (error)
            *error = "pick a username first.";
        return false;
    }
    if (password.isEmpty()) {
        if (error)
            *error = "pick a password.";
        return false;
    }
    if (password != confirmPassword) {
        if (error)
            *error = "the two passwords don't match.";
        return false;
    }

    QSqlQuery exists;
    exists.prepare("select count(*) from users where username = ?");
    exists.addBindValue(name);
    if (!exists.exec() || !exists.next()) {
        if (error)
            *error = "could not check the username: " + exists.lastError().text();
        return false;
    }
    if (exists.value(0).toLongLong() > 0) {
        if (error)
            *error = "that username is already taken.";
        return false;
    }

    QSqlQuery insert;
    insert.prepare(
        "insert into users (username, password_hash, role, display_name) "
        "values (?, ?, ?, ?)");
    insert.addBindValue(name);
    insert.addBindValue(hashPassword(password));
    insert.addBindValue("user");
    insert.addBindValue(name);

    if (!insert.exec()) {
        if (error)
            *error = "could not create the account: " + insert.lastError().text();
        return false;
    }

    return true;
}

AuthUser AuthService::login(const QString& username, const QString& password,
                            bool remember, QString* error)
{
    AuthUser user;

    QSqlQuery find;
    find.prepare(
        "select id, username, display_name, role, password_hash, is_active "
        "from users where username = ?");
    find.addBindValue(username.trimmed());

    if (!find.exec() || !find.next()) {
        if (error)
            *error = "wrong username or password.";
        return user;
    }

    const bool active = find.value("is_active").toBool();
    const QString stored = find.value("password_hash").toString();

    if (!active) {
        if (error)
            *error = "this account has been disabled.";
        return user;
    }

    if (!verifyPassword(password, stored)) {
        if (error)
            *error = "wrong username or password.";
        return user;
    }

    // one random token per login, we only store its hash
    const QString token = randomBytes(32).toHex();
    const QDateTime now = QDateTime::currentDateTimeUtc();

    QSqlQuery insert;
    insert.prepare(
        "insert into sessions (user_id, token_hash, expires_at, absolute_expires_at, user_agent) "
        "values (?, ?, ?, ?, ?) returning id");
    insert.addBindValue(find.value("id").toLongLong());
    insert.addBindValue(sha256Hex(token.toUtf8()));
    insert.addBindValue(now.addSecs(idleMinutes * 60));
    insert.addBindValue(now.addSecs(maxLifeDays * 24 * 3600));
    insert.addBindValue("asalancountdown-desktop");

    if (!insert.exec() || !insert.next()) {
        if (error)
            *error = "could not start a session: " + insert.lastError().text();
        return user;
    }
    const qint64 sessionId = insert.value(0).toLongLong();

    // last_login_at, per the spec
    QSqlQuery touch;
    touch.prepare("update users set last_login_at = ?, updated_at = ? where id = ?");
    touch.addBindValue(now);
    touch.addBindValue(now);
    touch.addBindValue(find.value("id").toLongLong());
    touch.exec();

    // tidy up old sessions while we're here
    QSqlQuery cleanup;
    cleanup.exec(
        "delete from sessions where absolute_expires_at < now() or expires_at < now()");

    user.id = find.value("id").toLongLong();
    user.sessionId = sessionId;
    user.username = find.value("username").toString();
    user.displayName = find.value("display_name").toString();
    user.role = find.value("role").toString();
    user.token = token;

    if (remember)
        storeToken(user);
    else
        forgetToken();

    return user;
}

AuthUser AuthService::restoreSession()
{
    AuthUser user;

    QSettings settings;
    const QString token = settings.value("session/token").toString();
    if (token.isEmpty())
        return user;

    QSqlQuery find;
    find.prepare(
        "select u.id, u.username, u.display_name, u.role, u.is_active, s.id as session_id "
        "from sessions s join users u on u.id = s.user_id "
        "where s.token_hash = ? and s.revoked_at is null "
        "and s.expires_at > now() and s.absolute_expires_at > now()");
    find.addBindValue(sha256Hex(token.toUtf8()));

    if (!find.exec() || !find.next() || !find.value("is_active").toBool()) {
        forgetToken();
        return user;
    }

    // session still good, push the idle window forward
    if (!slideSession(find.value("session_id").toLongLong())) {
        forgetToken();
        return user;
    }

    user.id = find.value("id").toLongLong();
    user.sessionId = find.value("session_id").toLongLong();
    user.username = find.value("username").toString();
    user.displayName = find.value("display_name").toString();
    user.role = find.value("role").toString();
    user.token = token;
    return user;
}

bool AuthService::slideSession(qint64 sessionId)
{
    if (sessionId <= 0)
        return false;

    QSqlQuery slide;
    slide.prepare(QString(
        "update sessions set last_activity_at = now(), "
        "expires_at = least(now() + interval '%1 minutes', absolute_expires_at) "
        "where id = ? and revoked_at is null and absolute_expires_at > now() "
        "returning id").arg(idleMinutes));
    slide.addBindValue(sessionId);

    // no row came back -> revoked or past the 7 day limit
    return slide.exec() && slide.next();
}

void AuthService::logout(const AuthUser& user)
{
    if (user.sessionId > 0) {
        QSqlQuery revoke;
        revoke.prepare("update sessions set revoked_at = now() "
                       "where id = ? and revoked_at is null");
        revoke.addBindValue(user.sessionId);
        revoke.exec();
    } else if (!user.token.isEmpty()) {
        QSqlQuery revoke;
        revoke.prepare("update sessions set revoked_at = now() "
                       "where token_hash = ? and revoked_at is null");
        revoke.addBindValue(sha256Hex(user.token.toUtf8()));
        revoke.exec();
    }
    forgetToken();
}

QString AuthService::hashPassword(const QString& password)
{
    const QByteArray salt = randomBytes(16);
    const QByteArray derived = QPasswordDigestor::deriveKeyPbkdf2(
        QCryptographicHash::Sha256,
        password.toUtf8(),
        salt,
        pbkdf2Iterations,
        32);

    return QString("%1$%2$%3$%4")
        .arg(hashPrefix)
        .arg(pbkdf2Iterations)
        .arg(QString::fromLatin1(salt.toHex()))
        .arg(QString::fromLatin1(derived.toHex()));
}

bool AuthService::verifyPassword(const QString& password, const QString& stored)
{
    const QStringList parts = stored.split('$');
    if (parts.size() != 4 || parts[0] != hashPrefix)
        return false;

    const int iterations = parts[1].toInt();
    const QByteArray salt = QByteArray::fromHex(parts[2].toLatin1());
    const QByteArray expected = QByteArray::fromHex(parts[3].toLatin1());
    if (iterations <= 0 || salt.isEmpty() || expected.isEmpty())
        return false;

    const QByteArray derived = QPasswordDigestor::deriveKeyPbkdf2(
        QCryptographicHash::Sha256,
        password.toUtf8(),
        salt,
        iterations,
        expected.size());

    // constant time compare so we don't leak where it differs
    return QCryptographicHash::hash(derived, QCryptographicHash::Sha256)
        == QCryptographicHash::hash(expected, QCryptographicHash::Sha256);
}

void AuthService::storeToken(const AuthUser& user)
{
    QSettings settings;
    settings.setValue("session/token", user.token);
    settings.setValue("session/username", user.username);
}

void AuthService::forgetToken()
{
    QSettings settings;
    settings.remove("session/token");
    settings.remove("session/username");
}

QByteArray AuthService::randomBytes(int count)
{
    QByteArray bytes(count, Qt::Uninitialized);
    QRandomGenerator::system()->generate(bytes.begin(), bytes.end());
    return bytes;
}

QString AuthService::sha256Hex(const QByteArray& data)
{
    return QString::fromLatin1(
        QCryptographicHash::hash(data, QCryptographicHash::Sha256).toHex());
}
