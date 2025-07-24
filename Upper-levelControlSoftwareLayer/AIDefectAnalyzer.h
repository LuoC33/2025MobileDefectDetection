#pragma once

#include <QObject>
#include <QString>
#include <QVariantMap>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QFile>
#include <QTextStream>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QVector>

class AIDefectAnalyzer : public QObject
{
    Q_OBJECT
public:
    explicit AIDefectAnalyzer(QObject *parent = nullptr);
    ~AIDefectAnalyzer();

    Q_INVOKABLE void analyzeCSV(const QString &filePath);

signals:
    void analysisComplete(const QVariantMap &result);
    void analysisFailed(const QString &error);

private slots:
    void handleApiResponse(QNetworkReply *reply);

private:
    QNetworkAccessManager *m_networkManager;
    const QString m_apiKey = "";
    const QString m_apiUrl = "";
    const QString m_model = "gpt-4o-mini";

    QVector<QStringList> readCSVData(const QString &filePath);
    QString generateAIPrompt(const QVector<QStringList> &data);
    QVariantMap processAIResponse(const QJsonDocument &response);
    QByteArray buildRequestBody(const QString &prompt);
};
