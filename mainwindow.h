#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QTimer>
#include <QMouseEvent>
#include <QPainter>
#include <QPoint>
#include <QSet>

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

protected:
    void contextMenuEvent(QContextMenuEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private slots:
    void refresh();

private:
    enum class Mode {
        Dark,
        Light
    };

    void applyStyle();
    enum class View { Clock, Battery, Cpu, Mem };
    QSet<View> m_views{View::Clock};
    QList<double> m_cpuHistory, m_memHistory;
    double m_battery = -1;
    double cpuPercent();
    void drawMetric(QPainter &painter, View view, int top) const;
    quint64 m_total = 0;
    quint64 m_idle = 0;

    QLabel *m_timeLabel;
    QTimer *m_timer;

    Mode m_mode = Mode::Dark;

    QPoint m_pressPos;
    bool m_dragging = false;
};

#endif
