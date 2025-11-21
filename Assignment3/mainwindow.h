#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPoint>
#include <QSet>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void Mouse_Pressed();
    void showMousePosition(QPoint&);
    void on_clear_clicked();
    void draw_grid();
    std::vector<QPoint> draw_line_bresenham(QPoint, QPoint);

    void on_spinBox_valueChanged(int);
    void repaint();

    void paint(std::vector<QPoint>, QColor);
    void paint(QPoint, QColor, QPainter&);

    void draw_ellipse(QPoint, int, int);
    std::vector<QPoint> draw_ellipse_polar(QPoint, int, int);
    std::vector<QPoint> draw_ellipse_bresenham(QPoint, int, int);
    std::vector<QPoint> draw_ellipse_cartesian(QPoint, int, int);

    void on_draw_ellipse_clicked();

    void on_ellipse_type_currentIndexChanged(int index);

    void on_undo_clicked();

private:
    Ui::MainWindow *ui;
    void addPoint(int x, int y, int c = 1);

    QPoint lastPoint1, lastPoint2;
    int sc_x, sc_y;
    int org_x, org_y;
    int width, height;

    std::vector<std::pair<std::vector<QPoint>, QColor>> colormap;
    int gap;
    int fellipse;
};

#endif // MAINWINDOW_H
