#pragma once

#include <QDateTime>
#include <QString>
#include <QVector>

// the logged-in user for this run of the app
struct AuthUser {
    qint64 id = 0;
    qint64 sessionId = 0; // the sessions table row for this login
    QString username;
    QString displayName;
    QString role;
    QString token; // raw session token, we only keep the hash in the db
};

// one row for the admin users list
struct UserRow {
    qint64 id = 0;
    QString username;
    QString role;
    QString displayName;
    bool active = true;
    QDateTime lastLoginAt;
};

class AuthService
{
public:
    // to-do list says 30 min idle, 7 days max — SessionWatch uses these too
    static constexpr int idleMinutes = 30;
    static constexpr int maxLifeDays = 7;

    // creates the default admin account if the users table is empty
    // returns true if it actually created one
    bool seedDefaultUser();

    // creates a new account (hashed password). fills error on failure
    bool registerUser(const QString& username, const QString& password,
                      const QString& confirmPassword, QString* error);

    // on failure returns a user with id 0 and fills in error
    // remember = keep the session token on disk for the next run
    AuthUser login(const QString& username, const QString& password,
                   bool remember, QString* error);

    // re-uses the saved "remember me" token from a previous run
    AuthUser restoreSession();

    // day 7: settings page - checks the current password, stores the new
    // one hashed, kills the user's sessions (spec: password change
    // invalidates every session)
    bool changePassword(qint64 userId, const QString& currentPassword,
                        const QString& newPassword, const QString& confirmPassword,
                        QString* error);

    // pushes the idle deadline forward, returns false if the session
    // was revoked or already expired (i.e. we should log out)
    bool slideSession(qint64 sessionId);

    // revokes the session row for this token
    void logout(const AuthUser& user);

    // day 9: admin user management. enable/disable kicks the target's
    // sessions; you can't disable yourself
    QVector<UserRow> listUsers();
    bool setUserActive(qint64 targetId, bool active, qint64 actorId, QString* error);
    bool setUserRole(qint64 targetId, const QString& role, qint64 actorId, QString* error);

private:
    QString hashPassword(const QString& password);
    bool verifyPassword(const QString& password, const QString& stored);
    void storeToken(const AuthUser& user);
    QString loadRememberedToken();
    void forgetToken();
    QByteArray randomBytes(int count);
    QString sha256Hex(const QByteArray& data);
};
