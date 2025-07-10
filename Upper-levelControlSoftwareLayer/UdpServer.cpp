#include "UdpServer.h"
#include <QDebug>
#include <QHostAddress>
#include <QStandardPaths>
#include <QDir>
#include <QDateTime>
#include <QtEndian>

UdpServer::UdpServer(QObject *parent)
    : QObject(parent), m_socket(nullptr), m_port(9999), m_expectedSize(0)
{
    m_serverAddress = getLocalIPAddress();
}

UdpServer::~UdpServer()
{
    stopServer();
}

void UdpServer::setPort(int port)
{

    if (m_port != port) {
        m_port = port;
        emit portChanged();
    }
}

// 启动服务器方法实现
bool UdpServer::startServer()
{
    if (m_socket && m_socket->state() == QAbstractSocket::BoundState) {
        return true;
    }
    if (m_socket) {
        delete m_socket;
    }
    m_socket = new QUdpSocket(this);
    connect(m_socket, &QUdpSocket::readyRead, this, &UdpServer::onDataReady);
    if (m_socket->bind(QHostAddress::Any, m_port)) {
        qDebug() << "UDP服务器启动成功，监听端口:" << m_port;
        emit isRunningChanged();
        return true;
    } else {
        QString error = QString("UDP服务器启动失败: %1").arg(m_socket->errorString());
        qDebug() << error;
        emit errorOccurred(error);
        return false;
    }
}

void UdpServer::stopServer()
{
    if (m_socket) {
        m_socket->close();
        m_socket->deleteLater();
        m_socket = nullptr;
        qDebug() << "UDP服务器已停止";
        emit isRunningChanged();
    }
}


void UdpServer::onDataReady()
{

    while (m_socket->hasPendingDatagrams()) {
        QByteArray datagram;

        datagram.resize(m_socket->pendingDatagramSize());
        QHostAddress sender;
        quint16 senderPort;
        m_socket->readDatagram(datagram.data(), datagram.size(), &sender, &senderPort);
        if (datagram.size() > 0) {

            quint8 dataType = static_cast<quint8>(datagram[0]);

            switch (dataType) {
            case 0x01:
                if (datagram.size() >= 5) {
                    m_expectedSize = qFromBigEndian<qint32>(reinterpret_cast<const uchar*>(datagram.data() + 1));
                    m_imageBuffer.clear();
                    qDebug() << "开始接收图片，预期大小:" << m_expectedSize;
                }
                break;

            case 0x02:
                m_imageBuffer.append(datagram.mid(1));
                qDebug() << "接收图片数据片段，当前大小:" << m_imageBuffer.size();
                break;

            case 0x03:
                if (m_imageBuffer.size() == m_expectedSize) {
                    if (saveImage(m_imageBuffer)) {
                        qDebug() << "图片接收完成";
                    }
                } else {
                    qDebug() << "图片数据不完整，期望:" << m_expectedSize << "实际:" << m_imageBuffer.size();
                }
                m_imageBuffer.clear();
                m_expectedSize = 0;
                break;

            default:
                // 未知的数据类型
                qDebug() << "未知数据类型:" << dataType;
                break;
            }
        }
    }
}

bool UdpServer::saveImage(const QByteArray &imageData)
{
    QString picturesPath = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
    QDir dir(picturesPath + "/ReceivedImages");
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    QString fileName = QString("image_%1.jpg").arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss"));
    QString filePath = dir.absoluteFilePath(fileName);
    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(imageData);
        file.close();
        emit imageReceived(filePath);
        return true;
    } else {
        // 保存失败
        emit errorOccurred("保存图片失败: " + file.errorString());
        return false;
    }
}

QString UdpServer::getLocalIPAddress()
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
