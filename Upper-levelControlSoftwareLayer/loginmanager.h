#ifndef LOGINMANAGER_H
#define LOGINMANAGER_H

#include <QObject>
#include <QQmlEngine>

class LoginManager : public QObject
{
    Q_OBJECT
public:
    explicit LoginManager(QObject *parent = nullptr);

    // 供QML调用的登录方法
    Q_INVOKABLE void login(const QString &username, const QString &password);

signals:
    // 登录结果信号
    void loginResult(bool success, const QString &message);

private:
    // 验证用户凭据的私有方法
    bool validateCredentials(const QString &username, const QString &password);
};

#endif // LOGINMANAGER_H
