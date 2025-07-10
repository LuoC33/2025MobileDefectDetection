
#include "ChartDataManager.h"
#include <QDebug>
#include <QVariantMap>
#include <QRandomGenerator>
#include <algorithm>
#include <cmath>
const QList<QColor> ChartDataManager::s_colors = {
    QColor("#FF6B6B"), QColor("#4ECDC4"), QColor("#45B7D1"), QColor("#96CEB4"),
    QColor("#FFEAA7"), QColor("#DDA0DD"), QColor("#98D8C8"), QColor("#F7DC6F"),
    QColor("#BB8FCE"), QColor("#85C1E9"), QColor("#F8C471"), QColor("#82E0AA"),
    QColor("#F1948A"), QColor("#85C1E9"), QColor("#F4D03F"), QColor("#AED6F1")
};

ChartDataManager::ChartDataManager(QObject *parent)
    : QObject(parent)
    , m_startTime(QDateTime::currentDateTime())
    , m_currentTimeMode(Seconds_10)
    , m_xAxisMin(0)
    , m_xAxisMax(10)
    , m_xAxisTitle("时间 (秒)")
    , m_xAxisTickCount(11)
    , m_xAxisMinorTickCount(10)
    , m_yAxisMin(0)
    , m_yAxisMax(10)
    , m_yAxisTitle("数量")
    , m_yAxisTickCount(6)
    , m_chartMode(AutoScaleMode)
    , m_hasReceivedData(false)
    , m_viewWindowSize(60.0)  // 默认60秒窗口
    , m_viewOffset(0.0)
    , m_maxDataTime(0.0)
    , m_canPanLeft(false)
    , m_canPanRight(false)
{
    qDebug() << "ChartDataManager initialized at:" << m_startTime.toString("hh:mm:ss.zzz");
}

QVariantList ChartDataManager::lineSeriesData() const
{
    return m_lineSeriesData;
}

QVariantList ChartDataManager::pieSeriesData() const
{
    return m_pieSeriesData;
}

QVariantList ChartDataManager::barSeriesData() const
{
    return m_barSeriesData;
}

QStringList ChartDataManager::categories() const
{
    return m_categories;
}

void ChartDataManager::addData(const QString &label, int quantity, double area)
{
    QDateTime currentTime = QDateTime::currentDateTime();


    if (!m_hasReceivedData) {
        m_firstDataTime = currentTime.toString("hh:mm:ss");
        m_hasReceivedData = true;
        emit firstDataTimeChanged();
    }


    qint64 elapsedMs = m_startTime.msecsTo(currentTime);
    double elapsedSeconds = elapsedMs / 1000.0;


    if (m_chartMode == AutoScaleMode) {

        rescaleTimeAxis();
        double scaledTime = convertToCurrentTimeScale(elapsedSeconds);
        processDataPoint(label, quantity, area, scaledTime, currentTime);
    } else {

        processDataPoint(label, quantity, area, elapsedSeconds, currentTime);

        if (elapsedSeconds > m_maxDataTime) {
            m_maxDataTime = elapsedSeconds;
            emit maxDataTimeChanged();
        }
        updateFixedSecondsAxis();
    }

    updateAllCharts();
}

void ChartDataManager::processDataPoint(const QString &label, int quantity, double area,
                                        double scaledTime, const QDateTime &currentTime)
{

    if (!m_labelColors.contains(label)) {
        int colorIndex = m_labelColors.size() % s_colors.size();
        m_labelColors[label] = s_colors[colorIndex];

        if (!m_categories.contains(label)) {
            m_categories.append(label);
            emit categoriesChanged();
        }
    }

    m_accumulatedQuantity[label] += quantity;
    m_accumulatedArea[label] += area;

    QVariantMap dataPoint;
    dataPoint["x"] = scaledTime;
    dataPoint["y"] = m_accumulatedQuantity[label];
    dataPoint["timestamp"] = currentTime.toString("hh:mm:ss.zzz");

    if (!m_timeSeriesData.contains(label)) {
        m_timeSeriesData[label] = QVariantList();
    }
    m_timeSeriesData[label].append(dataPoint);

    updateOtherLabelsHorizontalLines(scaledTime, currentTime, label);

    emit dataPointAdded(label, scaledTime, m_accumulatedQuantity[label]);
}

void ChartDataManager::updateOtherLabelsHorizontalLines(double currentScaledTime,
                                                        const QDateTime &currentTime,
                                                        const QString &updatedLabel)
{
    for (auto it = m_accumulatedQuantity.begin(); it != m_accumulatedQuantity.end(); ++it) {
        const QString &otherLabel = it.key();
        if (otherLabel != updatedLabel) {
            QVariantMap horizontalPoint;
            horizontalPoint["x"] = currentScaledTime;
            horizontalPoint["y"] = it.value();
            horizontalPoint["timestamp"] = currentTime.toString("hh:mm:ss.zzz");

            m_timeSeriesData[otherLabel].append(horizontalPoint);
        }
    }
}

void ChartDataManager::rescaleTimeAxis()
{
    QDateTime currentTime = QDateTime::currentDateTime();
    qint64 elapsedMs = m_startTime.msecsTo(currentTime);
    double elapsedSeconds = elapsedMs / 1000.0;

    TimeAxisMode newMode = m_currentTimeMode;

    if (elapsedSeconds <= 10) {
        newMode = Seconds_10;
    } else if (elapsedSeconds <= 60) {
        newMode = Seconds_60;
    } else if (elapsedSeconds <= 600) {
        newMode = Minutes_10;
    } else {
        newMode = Minutes_Long;
    }

    if (newMode != m_currentTimeMode) {
        qDebug() << "Time axis mode changed from" << m_currentTimeMode << "to" << newMode
                 << "at" << elapsedSeconds << "seconds";

        m_modeTransitionTimes.append(currentTime);
        m_currentTimeMode = newMode;
        updateTimeAxis();
    }
}

double ChartDataManager::convertToCurrentTimeScale(double originalTime)
{
    switch (m_currentTimeMode) {
    case Seconds_10:
    case Seconds_60:
        return originalTime;
    case Minutes_10:
    case Minutes_Long:
        return originalTime / 60.0;
    default:
        return originalTime;
    }
}

void ChartDataManager::updateTimeAxis()
{
    switch (m_currentTimeMode) {
    case Seconds_10:
        m_xAxisMin = 0;
        m_xAxisMax = 10;
        m_xAxisTitle = "时间 (秒)";
        m_xAxisTickCount = 11;
        m_xAxisMinorTickCount = 10;
        break;
    case Seconds_60:
        m_xAxisMin = 0;
        m_xAxisMax = 60;
        m_xAxisTitle = "时间 (秒)";
        m_xAxisTickCount = 7;
        m_xAxisMinorTickCount = 10;
        break;
    case Minutes_10:
        m_xAxisMin = 0;
        m_xAxisMax = 10;
        m_xAxisTitle = "时间 (分钟)";
        m_xAxisTickCount = 11;
        m_xAxisMinorTickCount = 10;
        break;
    case Minutes_Long:
        m_xAxisMin = 0;
        m_xAxisMax = 60;
        m_xAxisTitle = "时间 (分钟)";
        m_xAxisTickCount = 7;
        m_xAxisMinorTickCount = 6;
        break;
    }
    emit xAxisChanged();
}

void ChartDataManager::updateFixedSecondsAxis()
{
    // 计算视图范围
    double viewStart = m_maxDataTime - m_viewOffset - m_viewWindowSize;
    double viewEnd = m_maxDataTime - m_viewOffset;

    // 确保视图范围不超出数据范围
    if (viewStart < 0) {
        viewStart = 0;
        viewEnd = m_viewWindowSize;
    }

    m_xAxisMin = viewStart;
    m_xAxisMax = viewEnd;
    m_xAxisTitle = "时间 (秒)";

    // 根据窗口大小调整刻度
    if (m_viewWindowSize <= 60) {
        m_xAxisTickCount = static_cast<int>(m_viewWindowSize / 10) + 1;
        m_xAxisMinorTickCount = 10;
    } else if (m_viewWindowSize <= 300) {
        m_xAxisTickCount = static_cast<int>(m_viewWindowSize / 30) + 1;
        m_xAxisMinorTickCount = 6;
    } else {
        m_xAxisTickCount = static_cast<int>(m_viewWindowSize / 60) + 1;
        m_xAxisMinorTickCount = 6;
    }

    updatePanAvailability();
    emit xAxisChanged();
}

void ChartDataManager::updatePanAvailability()
{
    bool oldCanPanLeft = m_canPanLeft;
    bool oldCanPanRight = m_canPanRight;

    // 可以向左拖拽：当前视图的右边界小于最大数据时间
    m_canPanRight = (m_maxDataTime - m_viewOffset) < m_maxDataTime;

    // 可以向右拖拽：当前视图的左边界大于0
    m_canPanLeft = (m_maxDataTime - m_viewOffset - m_viewWindowSize) > 0;

    if (oldCanPanLeft != m_canPanLeft || oldCanPanRight != m_canPanRight) {
        emit panAvailabilityChanged();
    }
}

void ChartDataManager::updateValueAxis()
{
    if (m_accumulatedQuantity.isEmpty()) {
        m_yAxisMin = 0;
        m_yAxisMax = 10;
        m_yAxisTickCount = 6;
        emit yAxisChanged();
        return;
    }

    int maxQuantity = 0;
    for (auto it = m_accumulatedQuantity.begin(); it != m_accumulatedQuantity.end(); ++it) {
        maxQuantity = std::max(maxQuantity, it.value());
    }

    m_yAxisMin = 0;

    // Y轴优化：当数据达到当前最大值的90%时，将最大值增大1.2倍
    if (maxQuantity >= m_yAxisMax * 0.9) {
        m_yAxisMax = std::max(10.0, maxQuantity * 1.2);
    } else if (m_yAxisMax == 10 && maxQuantity > 0) {
        m_yAxisMax = std::max(10.0, maxQuantity * 1.2);
    }

    // 计算合适的刻度数量
    if (m_yAxisMax <= 10) {
        m_yAxisTickCount = static_cast<int>(m_yAxisMax) + 1;
    } else if (m_yAxisMax <= 50) {
        m_yAxisTickCount = 6;
    } else {
        m_yAxisTickCount = 6;
    }

    emit yAxisChanged();
}

void ChartDataManager::updateAllCharts()
{
    updateLineChart();
    updatePieChart();
    updateBarChart();
    updateValueAxis();
}

void ChartDataManager::updateLineChart()
{
    m_lineSeriesData.clear();

    for (auto it = m_timeSeriesData.begin(); it != m_timeSeriesData.end(); ++it) {
        const QString &label = it.key();
        const QVariantList &points = it.value();

        QVariantMap seriesData;
        seriesData["name"] = label;
        seriesData["color"] = m_labelColors[label];

        QVariantList filteredPoints;

        if (m_chartMode == FixedSecondsMode) {
            // 固定秒模式：只显示当前视图窗口内的数据
            double viewStart = m_maxDataTime - m_viewOffset - m_viewWindowSize;
            double viewEnd = m_maxDataTime - m_viewOffset;

            for (const QVariant &pointVar : points) {
                QVariantMap point = pointVar.toMap();
                double x = point["x"].toDouble();
                if (x >= viewStart && x <= viewEnd) {
                    filteredPoints.append(point);
                }
            }
        } else {
            // 自动模式：显示所有数据
            filteredPoints = points;
        }

        seriesData["data"] = filteredPoints;
        m_lineSeriesData.append(seriesData);
    }

    emit lineSeriesDataChanged();
}

void ChartDataManager::updatePieChart()
{
    m_pieSeriesData.clear();

    int totalQuantity = getTotalQuantity();
    if (totalQuantity == 0) {
        emit pieSeriesDataChanged();
        return;
    }

    for (auto it = m_accumulatedQuantity.begin(); it != m_accumulatedQuantity.end(); ++it) {
        const QString &label = it.key();
        int quantity = it.value();

        if (quantity > 0) {
            QVariantMap sliceData;
            sliceData["label"] = label;
            sliceData["value"] = quantity;
            sliceData["color"] = m_labelColors[label];
            sliceData["percentage"] = (double)quantity / totalQuantity * 100.0;

            m_pieSeriesData.append(sliceData);
        }
    }

    emit pieSeriesDataChanged();
}

void ChartDataManager::updateBarChart()
{
    m_barSeriesData.clear();

    for (auto it = m_accumulatedQuantity.begin(); it != m_accumulatedQuantity.end(); ++it) {
        const QString &label = it.key();
        int quantity = it.value();

        QVariantMap barData;
        barData["label"] = label;
        barData["value"] = quantity;
        barData["color"] = m_labelColors[label];

        m_barSeriesData.append(barData);
    }

    emit barSeriesDataChanged();
}

void ChartDataManager::setChartMode(ChartMode mode)
{
    if (m_chartMode == mode) return;

    qDebug() << "Chart mode changed from" << m_chartMode << "to" << mode;

    ChartMode oldMode = m_chartMode;
    m_chartMode = mode;

    if (mode == FixedSecondsMode) {
        // 切换到固定秒模式
        resetView();  // 重置到显示最新数据
        updateFixedSecondsAxis();
    } else {
        // 切换到自动模式
        updateTimeAxis();
    }

    updatePanAvailability();
    updateAllCharts();
    emit chartModeChanged();
}

void ChartDataManager::panLeft()
{
    if (m_chartMode != FixedSecondsMode || !m_canPanLeft) return;

    // 向左拖拽：减少偏移量，查看更早的数据
    double panStep = m_viewWindowSize * 0.5;  // 每次移动半个窗口
    double maxOffset = m_maxDataTime - m_viewWindowSize;

    m_viewOffset = qMin(maxOffset, m_viewOffset + panStep);

    qDebug() << "Pan left - new offset:" << m_viewOffset << "max data time:" << m_maxDataTime;

    updateFixedSecondsAxis();
    updateAllCharts();
}

void ChartDataManager::panRight()
{
    if (m_chartMode != FixedSecondsMode || !m_canPanRight) return;

    // 向右拖拽：增加偏移量，查看更新的数据
    double panStep = m_viewWindowSize * 0.5;  // 每次移动半个窗口

    m_viewOffset = qMax(0.0, m_viewOffset - panStep);

    qDebug() << "Pan right - new offset:" << m_viewOffset << "max data time:" << m_maxDataTime;

    updateFixedSecondsAxis();
    updateAllCharts();
}

void ChartDataManager::resetView()
{
    if (m_chartMode != FixedSecondsMode) return;

    // 重置到显示最新数据
    m_viewOffset = 0.0;  // 偏移为0表示显示最新数据

    qDebug() << "Reset view - offset:" << m_viewOffset << "window size:" << m_viewWindowSize;

    updateFixedSecondsAxis();
    updateAllCharts();
}

void ChartDataManager::setViewOffset(double offset)
{
    if (m_chartMode != FixedSecondsMode) return;

    // 限制偏移范围
    double maxOffset = qMax(0.0, m_maxDataTime - m_viewWindowSize);
    offset = qMax(0.0, qMin(maxOffset, offset));

    if (qAbs(m_viewOffset - offset) < 0.001) return;  // 避免微小变化

    m_viewOffset = offset;

    qDebug() << "Set view offset:" << m_viewOffset << "max offset:" << maxOffset;

    updateFixedSecondsAxis();
    updateAllCharts();
}

void ChartDataManager::setViewWindowSize(double windowSize)
{
    if (m_chartMode != FixedSecondsMode) return;

    // 限制窗口大小范围：10秒到3600秒（1小时）
    windowSize = qMax(10.0, qMin(3600.0, windowSize));

    if (qAbs(m_viewWindowSize - windowSize) < 0.001) return;

    double oldWindowSize = m_viewWindowSize;
    m_viewWindowSize = windowSize;

    // 调整偏移量以保持视图中心位置
    double centerTime = m_maxDataTime - m_viewOffset - oldWindowSize / 2.0;
    m_viewOffset = qMax(0.0, m_maxDataTime - centerTime - m_viewWindowSize / 2.0);

    qDebug() << "Set window size:" << m_viewWindowSize << "new offset:" << m_viewOffset;

    updateFixedSecondsAxis();
    updateAllCharts();
    emit viewWindowSizeChanged();
}

void ChartDataManager::clearData()
{
    qDebug() << "Clearing all chart data";

    // 清除所有数据容器
    m_timeSeriesData.clear();
    m_accumulatedQuantity.clear();
    m_accumulatedArea.clear();
    m_categories.clear();
    m_labelColors.clear();
    m_lineSeriesData.clear();
    m_pieSeriesData.clear();
    m_barSeriesData.clear();
    m_modeTransitionTimes.clear();

    // 重置时间基准
    m_startTime = QDateTime::currentDateTime();
    m_hasReceivedData = false;
    m_firstDataTime.clear();  // 修正：现在m_firstDataTime是QString，可以使用clear()

    // 重置坐标轴
    m_currentTimeMode = Seconds_10;
    m_xAxisMin = 0;
    m_xAxisMax = 10;
    m_xAxisTitle = "时间 (秒)";
    m_xAxisTickCount = 11;
    m_xAxisMinorTickCount = 10;
    m_yAxisMin = 0;
    m_yAxisMax = 10;
    m_yAxisTitle = "数量";
    m_yAxisTickCount = 6;

    // 重置视图控制
    m_viewOffset = 0.0;
    m_maxDataTime = 0.0;
    m_canPanLeft = false;
    m_canPanRight = false;

    // 根据当前模式更新坐标轴
    if (m_chartMode == FixedSecondsMode) {
        updateFixedSecondsAxis();
    } else {
        updateTimeAxis();
    }

    // 发送所有变更信号
    emit lineSeriesDataChanged();
    emit pieSeriesDataChanged();
    emit barSeriesDataChanged();
    emit categoriesChanged();
    emit xAxisChanged();
    emit yAxisChanged();
    emit firstDataTimeChanged();
    emit panAvailabilityChanged();
    emit maxDataTimeChanged();
}


int ChartDataManager::getTotalQuantity() const
{
    int total = 0;
    for (auto it = m_accumulatedQuantity.begin(); it != m_accumulatedQuantity.end(); ++it) {
        total += it.value();
    }
    return total;
}

double ChartDataManager::getTotalArea() const
{
    double total = 0.0;
    for (auto it = m_accumulatedArea.begin(); it != m_accumulatedArea.end(); ++it) {
        total += it.value();
    }
    return total;
}

int ChartDataManager::getQuantityForLabel(const QString &label) const
{
    return m_accumulatedQuantity.value(label, 0);
}

double ChartDataManager::getAreaForLabel(const QString &label) const
{
    return m_accumulatedArea.value(label, 0.0);
}

QColor ChartDataManager::getColorForLabel(const QString &label)
{
    if (!m_labelColors.contains(label)) {
        // 如果标签不存在，为其分配一个新颜色
        int colorIndex = m_labelColors.size() % s_colors.size();
        m_labelColors[label] = s_colors[colorIndex];
    }
    return m_labelColors.value(label, s_colors.first());
}
