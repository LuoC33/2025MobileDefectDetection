#ifndef UDPSERVER_H
#define UDPSERVER_H

#include <QObject>
#include <QUdpSocket>
#include <QNetworkInterface>    // 用于获取网络接口信息
#include <QPixmap>              // 用于处理图像数据
#include <QBuffer>              // 用于内存数据缓冲操作

// UdpServer类定义，继承自QObject
class UdpServer : public QObject
{
    Q_OBJECT  // Qt元对象宏，启用信号槽机制和属性系统

    // Qt属性定义，可在QML中访问
    Q_PROPERTY(bool isRunning READ isRunning NOTIFY isRunningChanged)  // 服务器运行状态属性
    Q_PROPERTY(int port READ port WRITE setPort NOTIFY portChanged)    // 服务器端口属性
    Q_PROPERTY(QString serverAddress READ serverAddress NOTIFY serverAddressChanged)  // 服务器地址属性

public:
    // 构造函数，parent参数默认为nullptr
    explicit UdpServer(QObject *parent = nullptr);
    // 析构函数
    ~UdpServer();

    // QML可调用方法，用于启动服务器
    Q_INVOKABLE bool startServer();
    // QML可调用方法，用于停止服务器
    Q_INVOKABLE void stopServer();

    // 属性访问器方法
    // 判断服务器是否正在运行（套接字存在且已绑定）
    bool isRunning() const { return m_socket && m_socket->state() == QAbstractSocket::BoundState; }
    // 获取端口号
    int port() const { return m_port; }
    // 设置端口号
    void setPort(int port);
    // 获取服务器IP地址
    QString serverAddress() const { return m_serverAddress; }

signals:
    // 当服务器运行状态改变时发射
    void isRunningChanged();
    // 当端口改变时发射
    void portChanged();
    // 当服务器地址改变时发射
    void serverAddressChanged();
    // 当接收到图像并保存成功时发射，包含图像保存路径
    void imageReceived(const QString &imagePath);
    // 当发生错误时发射
    void errorOccurred(const QString &error);

private slots:
    // 处理数据接收的槽函数
    void onDataReady();

private:
    // 获取本地IP地址
    QString getLocalIPAddress();
    // 保存接收到的图像数据
    bool saveImage(const QByteArray &imageData);

    QUdpSocket *m_socket;      // UDP套接字对象
    int m_port;                // 服务器端口
    QString m_serverAddress;   // 服务器地址
    QByteArray m_imageBuffer;  // 用于拼接分片的图片数据
    int m_expectedSize;        // 期望的图片总大小
};

#endif // UDPSERVER_H
