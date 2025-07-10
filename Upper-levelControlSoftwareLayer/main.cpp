#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include "LoginManager.h"
#include "TcpServer.h"
#include "UdpServer.h"
#include "ChartDataManager.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    qmlRegisterType<LoginManager>("loging.h", 1, 0, "LoginManager");
    qmlRegisterType<TcpServer>("Network", 1, 0, "TcpServer");
    qmlRegisterType<UdpServer>("Network", 1, 0, "UdpServer");
    qmlRegisterType<ChartDataManager>("Charts", 1, 0, "ChartDataManager");
    QQmlApplicationEngine engine;
    TcpServer tcpServer;
    UdpServer udpServer;
    ChartDataManager chartManager;
    QObject::connect(&tcpServer, &TcpServer::dataReceived,
                     &chartManager, &ChartDataManager::addData);
    QObject::connect(&tcpServer, &TcpServer::dataReceived,
                     [](const QString &label, int quantity, double area) {
                         qDebug() << "TCP数据接收:" << label << "数量:" << quantity << "面积:" << area;
                     });

    engine.rootContext()->setContextProperty("tcpServer", &tcpServer);
    engine.rootContext()->setContextProperty("udpServer", &udpServer);
    engine.rootContext()->setContextProperty("chartManager", &chartManager);
    const QUrl url(QStringLiteral("qrc:/main.qml"));
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url](QObject *obj, const QUrl &objUrl) {
                         if (!obj && url == objUrl) {
                             QCoreApplication::exit(-1);
                         }
                     }, Qt::QueuedConnection);

    engine.load(url);

    return app.exec();
}
