#ifndef USER_H
#define USER_H

#include <QString>

enum class UserRole
{
    Administrator,
    Cashier
};

enum class AccountStatus
{
    Enabled,
    Disabled
};

class User
{
public:
    User()
        : m_userId(0)
        , m_role(UserRole::Cashier)
        , m_status(AccountStatus::Disabled)
    {
    }

    User(int userId,
         const QString &username,
         const QString &password,
         UserRole role,
         AccountStatus status)
        : m_userId(userId)
        , m_username(username)
        , m_password(password)
        , m_role(role)
        , m_status(status)
    {
    }

    int userId() const
    {
        return m_userId;
    }

    QString username() const
    {
        return m_username;
    }

    QString password() const
    {
        return m_password;
    }

    UserRole role() const
    {
        return m_role;
    }

    AccountStatus status() const
    {
        return m_status;
    }

    bool isAdministrator() const
    {
        return m_role == UserRole::Administrator;
    }

    bool isEnabled() const
    {
        return m_status == AccountStatus::Enabled;
    }

private:
    int m_userId;
    QString m_username;
    QString m_password;
    UserRole m_role;
    AccountStatus m_status;
};

#endif // USER_H
