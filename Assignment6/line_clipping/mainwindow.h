#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QList>
#include <QPoint>
#include <QPointF>
#include <QRect>
#include "gridscene.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onDrawLine();
    void onRestoreLine();
    void onEraseLine();
    void onDrawClippingWindow();
    void onEraseClippingWindow();
    void onClipLineCohenSutherland();
    void onClipLineLiangBarsky();
    void onClearAll();
    void onCellClicked(const QPoint& cell);

private:
    Ui::MainWindow *ui;
    GridScene *scene;

    QList<QPoint> linePoints;
    QList<QPoint> originalLinePoints;
    bool isDrawingLine;
    int lineClickCount;

    QRect clippingWindow;
    bool hasClippingWindow;
    bool isDrawingWindow;
    int windowClickCount;
    QPoint windowStart;

    void bresenhamLine(const QPoint& p1, const QPoint& p2, const QBrush& brush);
    void drawRectangle(const QRect& rect, const QBrush& brush, int thickness = 1);
    void fillWindow(const QRect& rect, const QColor& color, int alpha);
    void clearLine();
    void clearWindow();
    void drawLineWithClipping(const QPoint& p1, const QPoint& p2, const QBrush& originalBrush, const QBrush& clippedBrush, bool useCohenSutherland);

    enum OutCode { INSIDE = 0, LEFT = 1, RIGHT = 2, BOTTOM = 4, TOP = 8 };

    int computeOutCode(double x, double y, const QRect& rect);
    bool cohenSutherlandClip(double& x1, double& y1, double& x2, double& y2, const QRect& rect);
    bool liangBarskyClip(double& x1, double& y1, double& x2, double& y2, const QRect& rect);

    void drawPartialLine(const QPoint& p1, const QPoint& p2, const QRect& window, const QBrush& insideBrush, const QBrush& outsideBrush, bool useCohenSutherland);
    bool isPointInsideWindow(const QPoint& p, const QRect& window);
};

#endif // MAINWINDOW_H
