#include "mainwindow.h"
#include <QTime>
#include <QFont>
#include <QApplication>
#include <QWindow>
#include <QContextMenuEvent>
#include <QDir>
#include <QFile>
#include <QMenu>

namespace {
constexpr int historySize = 60;
constexpr int rowHeight = 40;

void appendSample(QList<double> &history, double sample)
{
    if (history.size() == historySize) history.removeFirst();
    history.append(sample);
}

QByteArray firstLine(const QString &path)
{
    QFile file(path);
    return file.open(QIODevice::ReadOnly) ? file.readLine().trimmed() : QByteArray();
}

double batteryPercent()
{
    const QDir supplies("/sys/class/power_supply");
    int sum = 0, count = 0;
    for (const auto &name : supplies.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        const QString base = supplies.filePath(name) + "/";
        if (firstLine(base + "type") != "Battery" || firstLine(base + "scope") == "Device"
            || firstLine(base + "present") == "0")
            continue;
        bool ok;
        int capacity = firstLine(base + "capacity").toInt(&ok);
        if (ok && capacity >= 0 && capacity <= 100) {
            sum += capacity;
            ++count;
        }
    }
    return count ? double(sum) / count : -1;
}

double memoryPercent()
{
    QFile file("/proc/meminfo");
    if (!file.open(QIODevice::ReadOnly))
        return -1;
    qint64 total = 0, available = -1;
    for (const auto &line : file.readAll().split('\n')) {
        const auto fields = line.simplified().split(' ');
        if (fields.value(0) == "MemTotal:") total = fields.value(1).toLongLong();
        if (fields.value(0) == "MemAvailable:") available = fields.value(1).toLongLong();
    }
    return total > 0 && available >= 0 ? 100.0 * (total - available) / total : -1;
}
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_TranslucentBackground);

    m_timeLabel = new QLabel(this);
    m_timeLabel->setAlignment(Qt::AlignCenter);
    m_timeLabel->setAttribute(Qt::WA_TransparentForMouseEvents);

    QFont font("Segoe UI", 16, QFont::Bold);
    m_timeLabel->setFont(font);

    m_timeLabel->setGeometry(0, 0, 150, rowHeight);
    setFixedSize(150, rowHeight);

    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &MainWindow::refresh);
    m_timer->start(1000);

    refresh();
    applyStyle();
}

void MainWindow::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu menu(this);
    const QStringList names{"Clock", "Battery", "CPU", "Mem"};
    for (int i = 0; i < names.size(); ++i) {
        auto *action = menu.addAction(names[i]);
        action->setCheckable(true);
        action->setChecked(m_views.contains(View(i)));
        action->setData(i);
    }
    auto *choice = menu.exec(event->globalPos());
    if (!choice) return;
    const View view = View(choice->data().toInt());
    if (choice->isChecked()) m_views.insert(view);
    else m_views.remove(view);
    m_timeLabel->setVisible(m_views.contains(View::Clock));
    setFixedHeight(rowHeight * qMax(1, int(m_views.size())));
    update();
}

void MainWindow::refresh()
{
    m_timeLabel->setText(QTime::currentTime().toString("HH:mm:ss"));
    if (m_views.contains(View::Battery)) m_battery = batteryPercent();
    appendSample(m_cpuHistory, cpuPercent());
    appendSample(m_memHistory, memoryPercent());
    update();
}

double MainWindow::cpuPercent()
{
    const auto fields = firstLine("/proc/stat").simplified().split(' ');
    if (fields.value(0) != "cpu" || fields.size() < 5) return -1;
    quint64 total = 0;
    // Guest time is already included in user and nice time.
    for (int i = 1; i <= 8; ++i) total += fields.value(i).toULongLong();
    const quint64 idle = fields[4].toULongLong() + fields.value(5).toULongLong();
    double percent = -1;
    if (m_total && total > m_total && idle >= m_idle)
        percent = qBound(0.0, 100.0 * (1.0 - double(idle - m_idle) / (total - m_total)), 100.0);
    m_total = total;
    m_idle = idle;
    return percent;
}

void MainWindow::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_pressPos = event->pos();
        m_dragging = false;
        event->accept();
    }
}

void MainWindow::mouseMoveEvent(QMouseEvent *event)
{
    if (!(event->buttons() & Qt::LeftButton))
        return;

    if ((event->pos() - m_pressPos).manhattanLength()
        < QApplication::startDragDistance())
        return;

    if (!m_dragging) {
        m_dragging = true;
        if (windowHandle())
            windowHandle()->startSystemMove();
    }
}

void MainWindow::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && !m_dragging) {
        // Toggle mode on click
        m_mode = (m_mode == Mode::Dark) ? Mode::Light : Mode::Dark;
        applyStyle();
        update();
    }
}

void MainWindow::applyStyle()
{
    if (m_mode == Mode::Dark) {
        m_timeLabel->setStyleSheet("color: white;");
    } else {
        m_timeLabel->setStyleSheet("color: black;");
    }
}

void MainWindow::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    QColor panelColor;
    if (m_mode == Mode::Dark) {
        panelColor = QColor(20, 20, 20, 160);
    } else {
        panelColor = QColor(245, 245, 245, 180);
    }

    painter.setBrush(panelColor);
    painter.setPen(Qt::NoPen);
    painter.drawRect(rect());
    int top = m_views.contains(View::Clock) ? rowHeight : 0;
    for (View view : {View::Battery, View::Cpu, View::Mem}) {
        if (m_views.contains(view)) {
            drawMetric(painter, view, top);
            top += rowHeight;
        }
    }
}

void MainWindow::drawMetric(QPainter &painter, View view, int top) const
{
    const auto &history = view == View::Cpu ? m_cpuHistory : m_memHistory;
    const double percent = view == View::Battery ? m_battery : history.constLast();
    const QColor ink = m_mode == Mode::Dark ? Qt::white : Qt::black;
    const QString name = view == View::Battery ? "Battery" : view == View::Cpu ? "CPU" : "Mem";
    painter.setPen(ink);
    const QRect header(6, top, width() - 12, rowHeight / 2);
    painter.drawText(header, Qt::AlignLeft | Qt::AlignVCenter, name);
    painter.drawText(header, Qt::AlignRight | Qt::AlignVCenter,
                     percent < 0 ? QStringLiteral("—") : QString::number(qRound(percent)) + "%");
    const QRect plot(6, header.bottom() + 2, header.width(), rowHeight - header.height() - 6);
    QColor track = ink;
    track.setAlpha(50);
    painter.fillRect(plot, track);
    if (view == View::Battery) {
        if (percent >= 0)
            painter.fillRect(plot.x(), plot.y(), qRound(plot.width() * percent / 100), plot.height(), ink);
        return;
    }
    painter.setRenderHint(QPainter::Antialiasing);
    QPointF previous;
    for (int i = 0; i < history.size(); ++i) {
        if (history[i] < 0) continue;
        const QPointF point(plot.right() - (history.size() - 1 - i) * (plot.width() - 1.0) / (historySize - 1),
                            plot.bottom() - history[i] * (plot.height() - 1) / 100);
        if (i && history[i - 1] >= 0) painter.drawLine(previous, point);
        else painter.drawPoint(point);
        previous = point;
    }
}
