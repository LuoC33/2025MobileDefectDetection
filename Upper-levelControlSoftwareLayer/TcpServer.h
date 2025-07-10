#ifndef TCPSERVER_H
#define TCPSERVER_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QNetworkInterface>
#include <QRegularExpression> // 用于正则表达式匹配
#include <QTimer>          // 提供定时器功能


class TcpServer : public QObject
{
    Q_OBJECT

    // Qt属性定义，可在QML中访问
    Q_PROPERTY(bool isRunning READ isRunning NOTIFY isRunningChanged)  // 服务器运行状态属性
    Q_PROPERTY(int port READ port WRITE setPort NOTIFY portChanged)    // 服务器端口属性
    Q_PROPERTY(QString serverAddress READ serverAddress NOTIFY serverAddressChanged)  // 服务器地址属性

public:
    // 构造函数，parent参数默认为nullptr
    explicit TcpServer(QObject *parent = nullptr);
    // 析构函数
    ~TcpServer();

    // QML可调用方法，用于启动服务器
    Q_INVOKABLE bool startServer();
    // QML可调用方法，用于停止服务器
    Q_INVOKABLE void stopServer();

    // 属性访问器方法
    bool isRunning() const { return m_server->isListening(); }  // 返回服务器是否正在运行
    int port() const { return m_port; }  // 获取端口号
    void setPort(int port);  // 设置端口号
    QString serverAddress() const { return m_serverAddress; }  // 获取服务器IP地址
    // 在TcpServer.h中添加以下方法声明
public slots:
    // 发送数据到所有客户端
    void sendDataToAllClients(const QString &data);

    // 发送数据到指定IP的客户端
    void sendDataToClient(const QString &clientIP, const QString &data);

    // 获取连接的客户端列表
    QStringList getConnectedClients() const;

signals:
    // 当服务器运行状态改变时发射
    void isRunningChanged();
    // 当端口改变时发射
    void portChanged();
    // 当服务器地址改变时发射
    void serverAddressChanged();
    // 当接收到数据并解析成功时发射
    void dataReceived(const QString &label, int quantity, double area);
    // 当客户端连接时发射
    void clientConnected(const QString &clientAddress);
    // 当客户端断开连接时发射
    void clientDisconnected(const QString &clientAddress);
    // 当发生错误时发射
    void errorOccurred(const QString &error);

private slots:
    // 处理新连接的槽函数
    void onNewConnection();
    // 处理客户端断开连接的槽函数
    void onClientDisconnected();
    // 处理数据接收的槽函数
    void onDataReady();

private:
    // 解析接收到的数据
    void parseData(const QString &data);
    // 获取本地IP地址
    QString getLocalIPAddress();

    // 私有成员变量
    QTcpServer *m_server;          // TCP服务器对象
    QList<QTcpSocket*> m_clients;  // 客户端连接列表
    int m_port;                    // 服务器端口
    QString m_serverAddress;       // 服务器地址
};

#endif // TCPSERVER_H
