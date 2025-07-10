#ifndef CHARTDATAMANAGER_H
#define CHARTDATAMANAGER_H

#include <QObject>
#include <QVariantList>
#include <QVariantMap>
#include <QStringList>
#include <QColor>
#include <QDateTime>
#include <QTimer>

class ChartDataManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantList lineSeriesData READ lineSeriesData NOTIFY lineSeriesDataChanged)
    Q_PROPERTY(QVariantList pieSeriesData READ pieSeriesData NOTIFY pieSeriesDataChanged)
    Q_PROPERTY(QVariantList barSeriesData READ barSeriesData NOTIFY barSeriesDataChanged)
    Q_PROPERTY(QStringList categories READ categories NOTIFY categoriesChanged)

    // 坐标轴相关属性
    Q_PROPERTY(double xAxisMin READ xAxisMin NOTIFY xAxisChanged)
    Q_PROPERTY(double xAxisMax READ xAxisMax NOTIFY xAxisChanged)
    Q_PROPERTY(QString xAxisTitle READ xAxisTitle NOTIFY xAxisChanged)
    Q_PROPERTY(int xAxisTickCount READ xAxisTickCount NOTIFY xAxisChanged)
    Q_PROPERTY(int xAxisMinorTickCount READ xAxisMinorTickCount NOTIFY xAxisChanged)

    Q_PROPERTY(double yAxisMin READ yAxisMin NOTIFY yAxisChanged)
    Q_PROPERTY(double yAxisMax READ yAxisMax NOTIFY yAxisChanged)
    Q_PROPERTY(QString yAxisTitle READ yAxisTitle NOTIFY yAxisChanged)
    Q_PROPERTY(int yAxisTickCount READ yAxisTickCount NOTIFY yAxisChanged)

    // 图表模式相关属性
    Q_PROPERTY(ChartMode chartMode READ chartMode NOTIFY chartModeChanged)
    Q_PROPERTY(QString firstDataTime READ firstDataTime NOTIFY firstDataTimeChanged)
    Q_PROPERTY(bool canPanLeft READ canPanLeft NOTIFY panAvailabilityChanged)
    Q_PROPERTY(bool canPanRight READ canPanRight NOTIFY panAvailabilityChanged)
    Q_PROPERTY(double viewWindowSize READ viewWindowSize NOTIFY viewWindowSizeChanged)
    Q_PROPERTY(double maxDataTime READ maxDataTime NOTIFY maxDataTimeChanged)

public:
    explicit ChartDataManager(QObject *parent = nullptr);

    // 图表模式枚举
    enum ChartMode {
        AutoScaleMode,      // 自动单位变化模式
        FixedSecondsMode    // 固定秒单位模式
    };
    Q_ENUM(ChartMode)

    // 数据访问方法
    QVariantList lineSeriesData() const;
    QVariantList pieSeriesData() const;
    QVariantList barSeriesData() const;
    QStringList categories() const;

    // 坐标轴访问方法
    double xAxisMin() const { return m_xAxisMin; }
    double xAxisMax() const { return m_xAxisMax; }
    QString xAxisTitle() const { return m_xAxisTitle; }
    int xAxisTickCount() const { return m_xAxisTickCount; }
    int xAxisMinorTickCount() const { return m_xAxisMinorTickCount; }

    double yAxisMin() const { return m_yAxisMin; }
    double yAxisMax() const { return m_yAxisMax; }
    QString yAxisTitle() const { return m_yAxisTitle; }
    int yAxisTickCount() const { return m_yAxisTickCount; }

    // 模式相关访问方法
    ChartMode chartMode() const { return m_chartMode; }
    QString firstDataTime() const { return m_firstDataTime; }  // 修正：返回QString
    bool canPanLeft() const { return m_canPanLeft; }
    bool canPanRight() const { return m_canPanRight; }
    double viewWindowSize() const { return m_viewWindowSize; }
    double maxDataTime() const { return m_maxDataTime; }

    // 统计信息方法
    Q_INVOKABLE int getTotalQuantity() const;
    Q_INVOKABLE double getTotalArea() const;
    Q_INVOKABLE int getQuantityForLabel(const QString &label) const;
    Q_INVOKABLE double getAreaForLabel(const QString &label) const;
    Q_INVOKABLE QColor getColorForLabel(const QString &label);

    // 模式和视图控制方法
    Q_INVOKABLE void setChartMode(ChartMode mode);
    Q_INVOKABLE void panLeft();
    Q_INVOKABLE void panRight();
    Q_INVOKABLE void resetView();  // 重置到最新数据视图
    Q_INVOKABLE void setViewOffset(double offset);
    Q_INVOKABLE void setViewWindowSize(double windowSize);

public slots:
    void addData(const QString &label, int quantity, double area);
    void clearData();

signals:
    void lineSeriesDataChanged();
    void pieSeriesDataChanged();
    void barSeriesDataChanged();
    void categoriesChanged();
    void xAxisChanged();
    void yAxisChanged();
    void dataPointAdded(const QString &seriesName, double x, double y);

    // 新增信号
    void chartModeChanged();
    void firstDataTimeChanged();
    void panAvailabilityChanged();
    void viewWindowSizeChanged();
    void maxDataTimeChanged();

private:
    // 时间轴模式枚举（仅用于自动模式）
    enum TimeAxisMode {
        Seconds_10,    // 0-10秒：10个大刻度，间隔1秒，10个小刻度(0.1秒)
        Seconds_60,    // 10-60秒：6个大刻度，间隔10秒，10个小刻度(1秒)
        Minutes_10,    // 1-10分钟：10个大刻度，间隔1分钟，10个小刻度(6秒)
        Minutes_Long   // 10分钟以上：6个大刻度，间隔10分钟，6个小刻度
    };

    // 数据更新方法
    void updateAllCharts();
    void updateLineChart();
    void updatePieChart();
    void updateBarChart();

    // 坐标轴管理方法
    void updateTimeAxis();
    void updateValueAxis();
    void rescaleTimeAxis();
    double convertToCurrentTimeScale(double originalTime);

    // 固定秒模式的轴管理方法
    void updateFixedSecondsAxis();
    void updatePanAvailability();

    // 数据处理方法
    void processDataPoint(const QString &label, int quantity, double area,
                          double scaledTime, const QDateTime &currentTime);

    // 水平线更新方法
    void updateOtherLabelsHorizontalLines(double currentScaledTime, const QDateTime &currentTime, const QString &updatedLabel);

    // 数据存储
    QMap<QString, QVariantList> m_timeSeriesData;
    QMap<QString, int> m_accumulatedQuantity;
    QMap<QString, double> m_accumulatedArea;
    QMap<QString, QColor> m_labelColors;
    QStringList m_categories;

    // 图表数据
    QVariantList m_lineSeriesData;
    QVariantList m_pieSeriesData;
    QVariantList m_barSeriesData;

    // 时间管理
    QDateTime m_startTime;
    QString m_firstDataTime;        // 修正：改为QString类型
    TimeAxisMode m_currentTimeMode;
    QList<QDateTime> m_modeTransitionTimes;

    // 坐标轴属性
    double m_xAxisMin;
    double m_xAxisMax;
    QString m_xAxisTitle;
    int m_xAxisTickCount;
    int m_xAxisMinorTickCount;

    double m_yAxisMin;
    double m_yAxisMax;
    QString m_yAxisTitle;
    int m_yAxisTickCount;

    // 图表模式相关属性
    ChartMode m_chartMode;
    bool m_hasReceivedData;     // 是否已接收过数据

    // 固定秒模式的视图控制
    double m_viewWindowSize;    // 视图窗口大小（秒）
    double m_viewOffset;        // 视图偏移量（秒）
    double m_maxDataTime;       // 最大数据时间
    bool m_canPanLeft;
    bool m_canPanRight;

    // 静态颜色列表
    static const QList<QColor> s_colors;
};

#endif // CHARTDATAMANAGER_H
