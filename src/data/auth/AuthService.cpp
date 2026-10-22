#include "src/data/auth/AuthService.hpp"

#include <sodium.h>

#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QPasswordDigestor>
#include <QSettings>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QSysInfo>
#include <QVariant>

#include "src/services/audit/AuditService.hpp"

namespace {

// argon2id via libsodium - interactive parameters (ops 2, 64 MB), the
// standard choice for a login prompt
void ensureSodium()
{
    static const int ready = sodium_init();
    Q_UNUSED(ready)
}

const int pbkdf2Iterations = 100000;
const char* hashPrefix = "pbkdf2-sha256";

// remember-me file: token sealed with a key derived from the machine
// identity (windows user + machine guid). stops the file being useful if
// it's copied to another machine; not a defence against a determined
// local attacker (decision logged in PROGRESS.md)
QByteArray localTokenKey()
{
    ensureSodium();

    QString material = qEnvironmentVariable("USERNAME");
#ifdef Q_OS_WIN
    QSettings registry("HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Cryptography",
                       QSettings::NativeFormat);
    material += '@' + registry.value("MachineGuid").toString();
#else
    material += '@' + QSysInfo::machineHostName();
#endif

    const QByteArray pass = material.toUtf8();
    QByteArray key(crypto_secretbox_KEYBYTES, '\0');
    static const char salt[crypto_pwhash_SALTBYTES] = "orbit-remember1";
    if (crypto_pwhash(reinterpret_cast<unsigned char*>(key.data()), key.size(),
                      pass.constData(), pass.size(),
                      reinterpret_cast<const unsigned char*>(salt),
                      crypto_pwhash_OPSLIMIT_INTERACTIVE,
                      crypto_pwhash_MEMLIMIT_INTERACTIVE,
                      crypto_pwhash_ALG_DEFAULT) != 0)
        return QByteArray();

    return key;
}

QString rememberFilePath()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
        + "/remember.bin";
}

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
        "values (?, ?, ?, ?) returning id");
    insert.addBindValue(name);
    insert.addBindValue(hashPassword(password));
    insert.addBindValue("user");
    insert.addBindValue(name);

    if (!insert.exec() || !insert.next()) {
        if (error)
            *error = "could not create the account: " + insert.lastError().text();
        return false;
    }

    const qint64 newId = insert.value(0).toLongLong();
    AuditService::log(newId, "account.register", "user", newId, name);
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
    insert.addBindValue("orbit-desktop");

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

    const QString token = loadRememberedToken();
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

bool AuthService::changePassword(qint64 userId, const QString& currentPassword,
                                 const QString& newPassword,
                                 const QString& confirmPassword, QString* error)
{
    if (userId <= 0) {
        if (error)
            *error = "no user signed in.";
        return false;
    }
    if (newPassword.isEmpty()) {
        if (error)
            *error = "pick a new password.";
        return false;
    }
    if (newPassword != confirmPassword) {
        if (error)
            *error = "the two passwords don't match.";
        return false;
    }

    QSqlQuery find;
    find.prepare("select password_hash from users where id = ?");
    find.addBindValue(userId);
    if (!find.exec() || !find.next()) {
        if (error)
            *error = "that account no longer exists.";
        return false;
    }

    if (!verifyPassword(currentPassword, find.value(0).toString())) {
        if (error)
            *error = "the current password is wrong.";
        return false;
    }

    QSqlQuery update;
    update.prepare("update users set password_hash = ?, updated_at = now() where id = ?");
    update.addBindValue(hashPassword(newPassword));
    update.addBindValue(userId);
    if (!update.exec()) {
        if (error)
            *error = "could not save the new password.";
        return false;
    }

    // kill every session for this user - password change invalidates
    // all of them per the spec
    QSqlQuery revoke;
    revoke.prepare(
        "update sessions set revoked_at = now() where user_id = ? "
        "and revoked_at is null");
    revoke.addBindValue(userId);
    revoke.exec();

    AuditService::log(userId, "account.password_change", "user", userId);
    return true;
}

QVector<UserRow> AuthService::listUsers()
{
    QVector<UserRow> users;

    QSqlQuery query;
    if (!query.exec(
            "select id, username, role, display_name, is_active, last_login_at "
            "from users order by username"))
        return users;

    while (query.next()) {
        UserRow user;
        user.id = query.value(0).toLongLong();
        user.username = query.value(1).toString();
        user.role = query.value(2).toString();
        user.displayName = query.value(3).toString();
        user.active = query.value(4).toBool();
        user.lastLoginAt = query.value(5).toDateTime();
        users.append(user);
    }

    return users;
}

bool AuthService::setUserActive(qint64 targetId, bool active, qint64 actorId,
                                QString* error)
{
    if (targetId <= 0) {
        if (error)
            *error = "no user selected.";
        return false;
    }
    // you can't switch yourself off - the spec's admin rule
    if (targetId == actorId && !active) {
        if (error)
            *error = "you can't disable your own account.";
        return false;
    }

    QSqlQuery query;
    query.prepare(
        "update users set is_active = ?, updated_at = now() where id = ?");
    query.addBindValue(active);
    query.addBindValue(targetId);

    if (!query.exec() || query.numRowsAffected() == 0) {
        if (error)
            *error = "could not update the account.";
        return false;
    }

    // disabling someone kicks their sessions, same as a password change
    if (!active) {
        QSqlQuery revoke;
        revoke.prepare(
            "update sessions set revoked_at = now() where user_id = ? "
            "and revoked_at is null");
        revoke.addBindValue(targetId);
        revoke.exec();
    }

    AuditService::log(actorId, active ? "account.enable" : "account.disable",
                      "user", targetId);
    return true;
}

bool AuthService::setUserRole(qint64 targetId, const QString& role,
                              qint64 actorId, QString* error)
{
    if (targetId <= 0) {
        if (error)
            *error = "no user selected.";
        return false;
    }
    if (role != "admin" && role != "user") {
        if (error)
            *error = "role must be admin or user.";
        return false;
    }

    QSqlQuery query;
    query.prepare(
        "update users set role = ?, updated_at = now() where id = ?");
    query.addBindValue(role);
    query.addBindValue(targetId);

    if (!query.exec() || query.numRowsAffected() == 0) {
        if (error)
            *error = "could not update the role.";
        return false;
    }

    AuditService::log(actorId, "account.role_change", "user", targetId, role);
    return true;
}

QString AuthService::hashPassword(const QString& password)
{
    ensureSodium();

    // crypto_pwhash_str packs the salt and parameters into the string
    QByteArray out(crypto_pwhash_STRBYTES, '\0');
    const QByteArray pass = password.toUtf8();
    if (crypto_pwhash_str(out.data(), pass.constData(), pass.size(),
                          crypto_pwhash_OPSLIMIT_INTERACTIVE,
                          crypto_pwhash_MEMLIMIT_INTERACTIVE) != 0)
        return QString();

    return QString::fromLatin1(out.constData());
}

bool AuthService::verifyPassword(const QString& password, const QString& stored)
{
    const QByteArray pass = password.toUtf8();

    // rows hashed on day 2 and later
    if (stored.startsWith("$argon2")) {
        ensureSodium();
        const QByteArray storedBytes = stored.toLatin1();
        return crypto_pwhash_str_verify(storedBytes.constData(),
                                        pass.constData(), pass.size()) == 0;
    }

    // rows seeded with the old pbkdf2 scheme still verify; they turn into
    // argon2 strings the next time the password is changed
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
        pass,
        salt,
        iterations,
        expected.size());

    // constant time compare so we don't leak where it differs
    return QCryptographicHash::hash(derived, QCryptographicHash::Sha256)
        == QCryptographicHash::hash(expected, QCryptographicHash::Sha256);
}

void AuthService::storeToken(const AuthUser& user)
{
    ensureSodium();

    const QByteArray key = localTokenKey();
    if (key.isEmpty())
        return;

    // secretbox: nonce + sealed token, magic header so we can spot a
    // mangled/corrupt file and just forget it
    QByteArray nonce(crypto_secretbox_NONCEBYTES, '\0');
    randombytes_buf(nonce.data(), nonce.size());

    const QByteArray message = user.token.toUtf8();
    QByteArray box(crypto_secretbox_MACBYTES + message.size(), '\0');
    if (crypto_secretbox_easy(reinterpret_cast<unsigned char*>(box.data()),
                              reinterpret_cast<const unsigned char*>(message.constData()),
                              message.size(),
                              reinterpret_cast<const unsigned char*>(nonce.constData()),
                              reinterpret_cast<const unsigned char*>(key.constData())) != 0)
        return;

    QDir().mkpath(QFileInfo(rememberFilePath()).absolutePath());
    QFile file(rememberFilePath());
    if (!file.open(QIODevice::WriteOnly))
        return;
    file.write("ORBT1");
    file.write(nonce);
    file.write(box);
}

QString AuthService::loadRememberedToken()
{
    ensureSodium();

    QFile file(rememberFilePath());
    if (!file.open(QIODevice::ReadOnly))
        return QString();

    const QByteArray raw = file.readAll();
    const int headerSize = 5 + crypto_secretbox_NONCEBYTES;
    if (!raw.startsWith("ORBT1") || raw.size() <= headerSize + crypto_secretbox_MACBYTES)
        return QString();

    const QByteArray key = localTokenKey();
    if (key.isEmpty())
        return QString();

    const QByteArray nonce = raw.mid(5, crypto_secretbox_NONCEBYTES);
    const QByteArray box = raw.mid(headerSize);
    QByteArray message(box.size() - crypto_secretbox_MACBYTES, '\0');
    if (crypto_secretbox_open_easy(
            reinterpret_cast<unsigned char*>(message.data()),
            reinterpret_cast<const unsigned char*>(box.constData()),
            box.size(),
            reinterpret_cast<const unsigned char*>(nonce.constData()),
            reinterpret_cast<const unsigned char*>(key.constData())) != 0)
        return QString();

    return QString::fromUtf8(message);
}

void AuthService::forgetToken()
{
    QFile::remove(rememberFilePath());
}

QByteArray AuthService::randomBytes(int count)
{
    ensureSodium();
    QByteArray bytes(count, '\0');
    randombytes_buf(bytes.data(), size_t(count));
    return bytes;
}

QString AuthService::sha256Hex(const QByteArray& data)
{
    return QString::fromLatin1(
        QCryptographicHash::hash(data, QCryptographicHash::Sha256).toHex());
}
