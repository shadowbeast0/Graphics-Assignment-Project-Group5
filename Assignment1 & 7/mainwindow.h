#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPoint>
#include <QPair>
#include <QVector>
#include <variant>

namespace Ui {
class MainWindow;
}

typedef enum {
    POINT,
    LINE_DDA,
    LINE_BSH,
    BEZIER_CURVE
} DrawableType;

typedef struct {
    QPoint p1;
    QPoint p2;
} line_t;

typedef struct {
    QVector<QPoint> control_points;
    int n;
} bezier_curve_t;

using Shape = std::variant<QPoint, line_t, bezier_curve_t>;

typedef struct {
    DrawableType type;
    Shape shape;
} Drawable;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void Mouse_Pressed();
    void showMousePosition(QPoint &pos);
    void on_clear_clicked();
    void drawGrid();
    void on_spin_box_valueChanged(int arg1);
    void on_spin_box_textChanged(const QString &arg1);
    void on_draw_line_dda_clicked();
    void on_draw_line_bresenham_clicked();
    void on_draw_bezier_clicked();
    void on_animate_check_checkStateChanged(const Qt::CheckState &arg1);

private:
    Ui::MainWindow *ui;
    void addPoint(int x, int y, const QColor &fillColor = Qt::red);
    void addPoints(const QVector<QPoint>& points, const QColor& fillColor = Qt::red);

    QVector<Drawable> drawables;
    QVector<QVector<QColor>> pixelMatrix;
    QVector<QPoint> pointsList;
    QVector<QPoint> bezier_ctrl_pts;
    QPoint lastPoint1;
    QPoint lastPoint2;
    QColor lastColor;
    int sc_x, sc_y;
    int org_x, org_y;
    int gap;
    int frameSize;
    int bezier_n1;
    int getBoxX(int x);
    int getBoxY(int y);
    int getFrameX(int x);
    int getFrameY(int y);
    int getMatrixX(int x);
    int getMatrixY(int y);
    bool inRange(const int x, const int y);
    bool firstSpinBoxChanged;
    bool animate;
    qint64 drawLineDDA(const QPoint& p1, const QPoint& p2, const bool draw = true);
    qint64 drawLineBresenham(const QPoint& p1, const QPoint& p2, const bool draw = true);
    qint64 drawBezierCurve(const QVector<QPoint>& control_points, const bool draw = true);
};

#endif // MAINWINDOW_H
