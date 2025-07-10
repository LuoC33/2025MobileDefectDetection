#include "TcpServer.h"
#include <QDebug>
#include <QHostAddress>


TcpServer::TcpServer(QObject *parent)
    : QObject(parent), m_server(new QTcpServer(this)), m_port(8888)
{

    m_serverAddress = getLocalIPAddress();

    connect(m_server, &QTcpServer::newConnection, this, &TcpServer::onNewConnection);
}


TcpServer::~TcpServer()
{
    stopServer();
}


void TcpServer::setPort(int port)
{
    if (m_port != port) {
        m_port = port;
        emit portChanged();
    }
}


bool TcpServer::startServer()
{
    if (m_server->isListening()) {
        return true;
    }

    if (m_server->listen(QHostAddress::Any, m_port)) {
        qDebug() << "TCP服务器启动成功，监听端口:" << m_port;
        emit isRunningChanged();
        return true;
    } else {
        QString error = QString("TCP服务器启动失败: %1").arg(m_server->errorString());
        qDebug() << error;
        emit errorOccurred(error);
        return false;
    }
}


void TcpServer::stopServer()
{
    if (m_server->isListening())
    {
        for (QTcpSocket *client : m_clients)
        {
            client->disconnectFromHost();
            client->deleteLater();
        }
        m_clients.clear();
        m_server->close();

        qDebug() << "TCP服务器已停止";
        emit isRunningChanged();
    }
}
void TcpServer::onNewConnection()
{
    while (m_server->hasPendingConnections())
    {
        QTcpSocket *client = m_server->nextPendingConnection();
        m_clients.append(client);
        connect(client, &QTcpSocket::readyRead, this, &TcpServer::onDataReady);
        connect(client, &QTcpSocket::disconnected, this, &TcpServer::onClientDisconnected);
        QString clientAddress = client->peerAddress().toString();
        qDebug() << "客户端连接:" << clientAddress;
        emit clientConnected(clientAddress);
    }
}
void TcpServer::onClientDisconnected()
{
    QTcpSocket *client = qobject_cast<QTcpSocket*>(sender());
    if (client) {
        QString clientAddress = client->peerAddress().toString();
        m_clients.removeOne(client);
        client->deleteLater();

        qDebug() << "客户端断开连接:" << clientAddress;
        emit clientDisconnected(clientAddress);
    }
}

void TcpServer::onDataReady()
{

    QTcpSocket *client = qobject_cast<QTcpSocket*>(sender());
    if (client)
    {
        QByteArray data = client->readAll();
        QString dataString = QString::fromUtf8(data);
        qDebug() << "接收到数据:" << dataString;
        parseData(dataString);
    }
}
void TcpServer::parseData(const QString &data)
{
    QRegularExpression regex(R"(\[([^:]+):(\d+),(\d+)\])");
    QRegularExpressionMatchIterator iterator = regex.globalMatch(data);

    // 遍历所有匹配项
    while (iterator.hasNext()) {
        QRegularExpressionMatch match = iterator.next();
        if (match.hasMatch()) {
            // 提取匹配到的分组
            QString label = match.captured(1);
            int quantity = match.captured(2).toInt();
            double area = match.captured(3).toDouble();
            // 发射数据接收信号
            emit dataReceived(label, quantity, area);
        }
    }
}


QString TcpServer::getLocalIPAddress()
{
    QList<QHostAddress> addresses = QNetworkInterface::allAddresses();
    for (const QHostAddress &address : addresses) {

        if (address.protocol() == QAbstractSocket::IPv4Protocol &&
            address != QHostAddress::LocalHost) {
            return address.toString();
        }
    }
    return "127.0.0.1";
}

void TcpServer::sendDataToAllClients(const QString &data)
{
    QByteArray dataBytes = data.toUtf8();
    for (QTcpSocket *client : m_clients) {
        if (client && client->state() == QTcpSocket::ConnectedState) {
            client->write(dataBytes);
            client->flush();
        }
    }
    qDebug() << "发送数据到所有客户端:" << data;
}

void TcpServer::sendDataToClient(const QString &clientIP, const QString &data)
{
    QByteArray dataBytes = data.toUtf8();
    for (QTcpSocket *client : m_clients) {
        if (client && client->state() == QTcpSocket::ConnectedState) {
            QString peerIP = client->peerAddress().toString();
            if (peerIP == clientIP) {
                client->write(dataBytes);
                client->flush();
                qDebug() << "发送数据到客户端" << clientIP << ":" << data;
                return;
            }
        }
    }
    qDebug() << "未找到客户端:" << clientIP;
}

QStringList TcpServer::getConnectedClients() const
{
    QStringList clients;
    for (QTcpSocket *client : m_clients) {
        if (client && client->state() == QTcpSocket::ConnectedState) {
            clients.append(client->peerAddress().toString());
        }
    }
    return clients;
}
