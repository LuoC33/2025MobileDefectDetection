// LoginManager.cpp
#include "LoginManager.h"
#include <QDebug>

LoginManager::LoginManager(QObject *parent)
    : QObject(parent)
{
}

void LoginManager::login(const QString &username, const QString &password)
{
    qDebug() << "收到登录请求 - 用户名:" << username;

    bool success = validateCredentials(username, password);
    QString message;

    if (success) {
        message = "登录成功！";
        qDebug() << "用户" << username << "登录成功";
    } else {
        message = "用户名或密码错误";
        qDebug() << "用户" << username << "登录失败";
    }


    emit loginResult(success, message);
}

bool LoginManager::validateCredentials(const QString &username, const QString &password)
{
    return (username == "admin" && password == "admin");
}
