#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QList>
#include <QPoint>
#include <QPointF>
#include <QRect>
#include <QSet>
#include <QMap>
#include <cmath>
#include "gridscene.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

struct VertexData {
    QPointF pos;
    bool isIntersection = false;
    bool isStartNode = false;
    int link = -1;
    bool visited = false;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onDrawPolygon();
    void onRestorePolygon();
    void onErasePolygon();
    void onDrawClippingWindow();
    void onEraseClippingWindow();
    void onClipPolygonSutherHodge();
    void onClipPolygonWeilerAther();
    void onClearAll();
    void onCellClicked(const QPoint& cell);

private:
    Ui::MainWindow *ui;
    GridScene *scene;

    QList<QPoint> polygonVertices;
    QList<QPoint> originalPolygonVertices;
    bool hasPolygon;
    bool isSelectingVertices;

    QRect clippingWindow;
    bool hasClippingWindow;
    bool isDrawingWindow;
    int windowClickCount;
    QPoint windowStart;

    void bresenhamLine(const QPoint& p1, const QPoint& p2, const QBrush& brush, bool collect = false);
    void drawPolygonOutline(const QList<QPoint>& vertices, const QBrush& brush, bool collect = false);
    void drawRectangle(const QRect& rect, const QBrush& brush);
    void drawRectangle(const QRect& rect, const QBrush& brush, int thickness);
    void clearPolygon();
    void clearWindow();
    void fillWindow(const QRect& rect, const QColor& color, int alpha);

    enum EdgePosition {
        LEFT_EDGE = 0,
        RIGHT_EDGE = 1,
        BOTTOM_EDGE = 2,
        TOP_EDGE = 3
    };

    QList<QPointF> clipAgainstEdge(const QList<QPointF>& inputVertices, EdgePosition edge, const QRect& clipRect);
    bool isInside(const QPointF& point, EdgePosition edge, const QRect& clipRect);
    QPointF getIntersection(const QPointF& p1, const QPointF& p2, EdgePosition edge, const QRect& clipRect);
    QList<QPointF> sutherlandHodgemanClip(const QList<QPointF>& polygon, const QRect& clipRect);
    QPointF computeLineIntersection(const QPointF& p1, const QPointF& p2, const QPointF& p3, const QPointF& p4);
    double distance(const QPointF& p1, const QPointF& p2);
    bool isPointInsidePolygon(const QPointF& point, const QList<QPointF>& polygon);
    void drawPolygonInsideRect(const QList<QPoint>& vertices, const QRect& rect, const QBrush& brush);


    QPointF getIntersectionWA(const QPointF& p1, const QPointF& p2, const QPointF& w1, const QPointF& w2);
    void weilerAthertonPolygonClip();
    bool isInsideClipWindow(const QPointF& p) const;
    bool pointInPolygon(const QVector<QPointF>& poly, const QPointF& test) const;
    QVector<QPoint> preClipPolygon;
    QVector<QPair<QPoint, QPoint>> preClipLines;
    bool hasPreClip = false;
};

#endif // MAINWINDOW_H
