#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "gridscene.h"
#include "gridview.h"
#include <QMessageBox>
#include <cmath>
#include <algorithm>

QSet<QPoint> polygonPixels;
static inline bool feq(double a, double b, double eps=1e-6){
    return std::abs(a-b)<=eps; }

static inline bool peq(const QPointF& a, const QPointF& b){
    return feq(a.x(),b.x()) && feq(a.y(),b.y());
}


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , scene(new GridScene(this))
    , hasPolygon(false)
    , isSelectingVertices(false)
    , hasClippingWindow(false)
    , isDrawingWindow(false)
    , windowClickCount(0)
{
    ui->setupUi(this);

    ui->grid->setScene(scene);
    ui->grid->setFixedSize(ui->grid->width(), ui->grid->height());
    ui->grid->centerOn(0, 0);
    scene->setCellSize(10);

    connect(ui->drawPolygon, &QPushButton::clicked, this, &MainWindow::onDrawPolygon);
    connect(ui->restorePolygon, &QPushButton::clicked, this, &MainWindow::onRestorePolygon);
    connect(ui->erasePolygon, &QPushButton::clicked, this, &MainWindow::onErasePolygon);
    connect(ui->drawWindow, &QPushButton::clicked, this, &MainWindow::onDrawClippingWindow);
    connect(ui->eraseWindow, &QPushButton::clicked, this, &MainWindow::onEraseClippingWindow);
    connect(ui->clipPolygonSutherHodge, &QPushButton::clicked, this, &MainWindow::onClipPolygonSutherHodge);
    connect(ui->clipPolygonWeilerAther, &QPushButton::clicked, this, &MainWindow::onClipPolygonWeilerAther);
    connect(ui->clearAll, &QPushButton::clicked, this, &MainWindow::onClearAll);
    connect(scene, &GridScene::leftClick, this, &MainWindow::onCellClicked);

    polygonVertices.clear();
    originalPolygonVertices.clear();
}

MainWindow::~MainWindow()
{
    delete scene;
    delete ui;
}

void MainWindow::onCellClicked(const QPoint& cell)
{
    if (isSelectingVertices) {
        polygonVertices.append(cell);
        scene->paintCell(cell, QBrush(Qt::blue));
        scene->update();
    }
    else if (isDrawingWindow) {
        if (windowClickCount == 0) {
            windowStart = cell;
            scene->paintCell(cell, QBrush(Qt::red));
            windowClickCount++;
        }
        else if (windowClickCount == 1) {
            int xMin = std::min(windowStart.x(), cell.x());
            int yMin = std::min(windowStart.y(), cell.y());
            int xMax = std::max(windowStart.x(), cell.x());
            int yMax = std::max(windowStart.y(), cell.y());
            clippingWindow = QRect(QPoint(xMin, yMin), QPoint(xMax, yMax));
            fillWindow(clippingWindow, QColor(173, 216, 230), 120);
            drawRectangle(clippingWindow, QBrush(Qt::red), 1);
            if (hasPolygon) drawPolygonOutline(polygonVertices, QBrush(Qt::blue));
            hasClippingWindow = true;
            isDrawingWindow = false;
            windowClickCount = 0;
            scene->update();
        }
    }
}


void MainWindow::onDrawPolygon()
{
    if (isDrawingWindow) {
        QMessageBox::warning(this, "Warning", "Please finish drawing the clipping window first!");
        return;
    }

    if (isSelectingVertices) {
        if (polygonVertices.size() < 3) {
            QMessageBox::warning(this, "Warning", "Please select at least 3 vertices for a polygon!");
            return;
        }

        originalPolygonVertices = polygonVertices;
        drawPolygonOutline(polygonVertices, QBrush(Qt::blue), true);
        hasPolygon = true;
        isSelectingVertices = false;
        scene->update();
    }
    else {
        clearPolygon();
        polygonVertices.clear();
        originalPolygonVertices.clear();
        isSelectingVertices = true;
        hasPolygon = false;
        QMessageBox::information(this, "Draw Polygon", "Click on the grid to select vertices. Click 'Draw Polygon' again to finish.");
    }
}

void MainWindow::onRestorePolygon()
{
    if (originalPolygonVertices.size() < 3) {
        QMessageBox::warning(this, "Warning", "No original polygon to restore!");
        return;
    }

    clearPolygon();
    polygonVertices = originalPolygonVertices;
    drawPolygonOutline(polygonVertices, QBrush(Qt::blue), true);
    hasPolygon = true;
    scene->update();
}


void MainWindow::onErasePolygon()
{
    clearPolygon();
    polygonVertices.clear();
    originalPolygonVertices.clear();
    hasPolygon = false;
    isSelectingVertices = false;
    scene->update();
}


void MainWindow::onDrawClippingWindow()
{
    if (isSelectingVertices) {
        QMessageBox::warning(this, "Warning", "Please finish drawing the polygon first!");
        return;
    }

    clearWindow();
    isDrawingWindow = true;
    windowClickCount = 0;
    hasClippingWindow = false;
    QMessageBox::information(this, "Draw Clipping Window", "Click two diagonal corners to define the clipping window.");
}


void MainWindow::onEraseClippingWindow()
{
    clearWindow();
    hasClippingWindow = false;
    scene->update();
}


void MainWindow::drawPolygonInsideRect(const QList<QPoint>& /*unused*/, const QRect& rect, const QBrush& brush)
{
    for (const QPoint& p : polygonPixels) {
        if (rect.contains(p, true)) scene->paintCell(p, brush);
    }
}


void MainWindow::onClipPolygonSutherHodge()
{
    if (!hasPolygon) { QMessageBox::warning(this, "Warning", "Please draw a polygon first!"); return; }
    if (!hasClippingWindow) { QMessageBox::warning(this, "Warning", "Please draw a clipping window first!"); return; }
    if (polygonVertices.size() < 3) { QMessageBox::warning(this, "Warning", "Invalid polygon!"); return; }

    QList<QPointF> inputPolygon;
    for (const QPoint& p : polygonVertices) inputPolygon.append(QPointF(p));
    QList<QPointF> clippedPolygon = sutherlandHodgemanClip(inputPolygon, clippingWindow);

    QList<QPoint> original = polygonVertices;
    clearPolygon();
    drawPolygonOutline(original, QBrush(Qt::gray), true);
    fillWindow(clippingWindow, QColor(173, 216, 230), 120);
    drawRectangle(clippingWindow, QBrush(Qt::red), 1);

    if (clippedPolygon.size() >= 2) {
        QList<QPoint> clippedInt;
        clippedInt.reserve(clippedPolygon.size());
        for (const QPointF& p : clippedPolygon)
            clippedInt.append(QPoint(qRound(p.x()), qRound(p.y())));

        drawPolygonOutline(clippedInt, QBrush(Qt::green));

        polygonVertices = clippedInt;
        hasPolygon = true;
    } else {
        polygonVertices.clear();
        hasPolygon = false;
        QMessageBox::information(this, "Clipping Result", "Polygon is completely outside the clipping window or too small after clipping!");
    }

    scene->update();
}


void MainWindow::onClipPolygonWeilerAther()
{
    if (!hasPolygon) { QMessageBox::warning(this, "Warning", "Please draw a polygon first!"); return; }
    if (!hasClippingWindow) { QMessageBox::warning(this, "Warning", "Please draw a clipping window first!"); return; }
    if (polygonVertices.size() < 3) { QMessageBox::warning(this, "Warning", "Invalid polygon!"); return; }

    weilerAthertonPolygonClip();
}


QPointF MainWindow::getIntersectionWA(const QPointF& p1, const QPointF& p2, const QPointF& q1, const QPointF& q2)
{
    double A1 = p2.y() - p1.y();
    double B1 = p1.x() - p2.x();
    double C1 = A1 * p1.x() + B1 * p1.y();

    double A2 = q2.y() - q1.y();
    double B2 = q1.x() - q2.x();
    double C2 = A2 * q1.x() + B2 * q1.y();

    double det = A1 * B2 - A2 * B1;
    if (std::abs(det) < 1e-9) return QPointF();

    double x = (B2 * C1 - B1 * C2) / det;
    double y = (A1 * C2 - A2 * C1) / det;

    const double eps = 1e-6;
    if (x >= std::min(p1.x(), p2.x()) - eps && x <= std::max(p1.x(), p2.x()) + eps &&
        y >= std::min(p1.y(), p2.y()) - eps && y <= std::max(p1.y(), p2.y()) + eps &&
        x >= std::min(q1.x(), q2.x()) - eps && x <= std::max(q1.x(), q2.x()) + eps &&
        y >= std::min(q1.y(), q2.y()) - eps && y <= std::max(q1.y(), q2.y()) + eps)
    {
        return QPointF(x, y);
    }
    return QPointF();
}


bool MainWindow::isInsideClipWindow(const QPointF& p) const
{
    return p.x() >= clippingWindow.left()  && p.x() <= clippingWindow.right() && p.y() >= clippingWindow.top()   && p.y() <= clippingWindow.bottom();
}

bool MainWindow::pointInPolygon(const QVector<QPointF>& poly, const QPointF& test) const
{
    bool inside = false;
    int n = poly.size();
    for (int i = 0, j = n - 1; i < n; j = i++) {
        const QPointF& pi = poly[i];
        const QPointF& pj = poly[j];
        bool intersect = ((pi.y() > test.y()) != (pj.y() > test.y())) &&
                         (test.x() < (pj.x() - pi.x()) * (test.y() - pi.y()) /
                                             (pj.y() - pi.y() + 1e-12) + pi.x());
        if (intersect) inside = !inside;
    }
    return inside;
}

void MainWindow::weilerAthertonPolygonClip()
{
    QVector<QPoint> origPolyInt = QVector<QPoint>::fromList(polygonVertices);
    QVector<QPointF> origPoly; origPoly.reserve(origPolyInt.size());
    for (const auto& q : origPolyInt) origPoly.append(QPointF(q));


    QList<VertexData> subjectList;
    for (const auto& p : polygonVertices) subjectList.append({QPointF(p)});

    QList<VertexData> clipList;
    QVector<QPoint> windowVerts = {
        clippingWindow.topLeft(),
        QPoint(clippingWindow.right(),  clippingWindow.top()),
        clippingWindow.bottomRight(),
        QPoint(clippingWindow.left(),   clippingWindow.bottom())
    };
    for (const auto& p : windowVerts) clipList.append({QPointF(p)});

    bool allInside = true;
    for (const auto& v : subjectList) {
        if (!isInsideClipWindow(v.pos)) {
            allInside = false; break;
        }
    }

    QList<QPair<int, VertexData>> subjectInserts, clipInserts;
    for (int i = 0; i < subjectList.size(); ++i) {
        QPointF p1 = subjectList[i].pos;
        QPointF p2 = subjectList[(i + 1) % subjectList.size()].pos;

        for (int j = 0; j < clipList.size(); ++j) {
            QPointF w1 = clipList[j].pos;
            QPointF w2 = clipList[(j + 1) % clipList.size()].pos;

            QPointF I = getIntersectionWA(p1, p2, w1, w2);
            if (!I.isNull()) {
                bool isStartNode = isInsideClipWindow(p1) && !isInsideClipWindow(p2);
                subjectInserts.append({ i + 1, { I, true, isStartNode, -1, false } });
                clipInserts.append   ({ j + 1, { I, true, isStartNode, -1, false } });
            }
        }
    }

    QList<QPoint> original = polygonVertices;
    clearPolygon();
    if (!original.isEmpty()) drawPolygonOutline(original, QBrush(Qt::lightGray), /*collect=*/true);
    fillWindow(clippingWindow, QColor(173, 216, 230), 120);
    drawRectangle(clippingWindow, QBrush(Qt::red), 1);


    if (subjectInserts.isEmpty()) {
        if (allInside) {
            if (!original.isEmpty()) drawPolygonOutline(original, QBrush(Qt::green));
        } else {
            QPointF center((clippingWindow.left()+clippingWindow.right())/2.0,
                           (clippingWindow.top() +clippingWindow.bottom())/2.0);
            if (!origPoly.isEmpty() && pointInPolygon(origPoly, center)) {
                drawRectangle(clippingWindow, QBrush(Qt::green));
            }
        }
        scene->update();
        return;
    }

    std::sort(subjectInserts.begin(), subjectInserts.end(),
              [](auto& a, auto& b){ return a.first > b.first; });
    for (auto& ins : subjectInserts) subjectList.insert(ins.first, ins.second);

    std::sort(clipInserts.begin(), clipInserts.end(),
              [](auto& a, auto& b){ return a.first > b.first; });
    for (auto& ins : clipInserts) clipList.insert(ins.first, ins.second);


    for (int i = 0; i < subjectList.size(); ++i) {
        if (!subjectList[i].isIntersection) continue;
        for (int j = 0; j < clipList.size(); ++j) {
            if (clipList[j].isIntersection && subjectList[i].pos == clipList[j].pos) {
                subjectList[i].link = j;
                clipList[j].link = i;
                break;
            }
        }
    }

    QList<QVector<QPointF>> resultPolygons;
    for (int i = 0; i < subjectList.size(); ++i) {
        if (subjectList[i].isIntersection && subjectList[i].isStartNode && !subjectList[i].visited) {
            QVector<QPointF> cur;
            int idx = i;
            bool onSubject = true;
            do {
                VertexData* v = onSubject ? &subjectList[idx] : &clipList[idx];
                v->visited = true;
                cur.append(v->pos);
                if (v->isIntersection && v->link != -1) { onSubject = !onSubject; idx = v->link; }
                idx = (idx + 1) % (onSubject ? subjectList.size() : clipList.size());
            } while (cur.first() != cur.back() || cur.size() < 2);
            cur.pop_back();
            if (cur.size() >= 3) resultPolygons.append(cur);
        }
    }


    if (!resultPolygons.isEmpty()) {
        for (const auto& poly : resultPolygons) {
            QList<QPoint> edge;
            edge.reserve(poly.size());
            for (const QPointF& p : poly) edge.append(QPoint(qRound(p.x()), qRound(p.y())));
            drawPolygonOutline(edge, QBrush(Qt::green));
        }
    }
    scene->update();
}



void MainWindow::onClearAll()
{
    scene->clearCells();
    polygonVertices.clear();
    originalPolygonVertices.clear();
    hasPolygon = false;
    isSelectingVertices = false;
    hasClippingWindow = false;
    isDrawingWindow = false;
    windowClickCount = 0;
    scene->update();
}


void MainWindow::bresenhamLine(const QPoint& p1, const QPoint& p2, const QBrush& brush, bool collect)
{
    int x1 = p1.x(), y1 = p1.y(), x2 = p2.x(), y2 = p2.y();
    int dx = abs(x2 - x1), dy = abs(y2 - y1);
    int sx = (x1 < x2) ? 1 : -1;
    int sy = (y1 < y2) ? 1 : -1;
    int err = dx - dy;

    while (true) {
        QPoint q(x1, y1);
        scene->paintCell(q, brush);
        if (collect) polygonPixels.insert(q);

        if (x1 == x2 && y1 == y2) break;
        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x1 += sx; }
        if (e2 < dx) { err += dx; y1 += sy; }
    }
}


void MainWindow::drawPolygonOutline(const QList<QPoint>& vertices, const QBrush& brush, bool collect)
{
    if (vertices.size() < 2) return;

    for (const QPoint& v : vertices) {
        scene->paintCell(v, brush);
        if (collect) polygonPixels.insert(v);
    }

    for (int i = 0; i < vertices.size(); i++) {
        int nextIndex = (i + 1) % vertices.size();
        bresenhamLine(vertices[i], vertices[nextIndex], brush, collect);
    }
}


void MainWindow::drawRectangle(const QRect& rect, const QBrush& brush)
{
    bresenhamLine(rect.topLeft(), rect.topRight(), brush);
    bresenhamLine(rect.topRight(), rect.bottomRight(), brush);
    bresenhamLine(rect.bottomRight(), rect.bottomLeft(), brush);
    bresenhamLine(rect.bottomLeft(), rect.topLeft(), brush);
}

void MainWindow::drawRectangle(const QRect& rect, const QBrush& brush, int thickness)
{
    for (int i = 0; i < thickness; ++i) {
        bresenhamLine(QPoint(rect.left() + i, rect.top() + i), QPoint(rect.right() - i, rect.top() + i), brush);
        bresenhamLine(QPoint(rect.left() + i, rect.bottom() - i), QPoint(rect.right() - i, rect.bottom() - i), brush);
        bresenhamLine(QPoint(rect.left() + i, rect.top() + i), QPoint(rect.left() + i, rect.bottom() - i), brush);
        bresenhamLine(QPoint(rect.right() - i, rect.top() + i), QPoint(rect.right() - i, rect.bottom() - i), brush);
    }
}


void MainWindow::fillWindow(const QRect& rect, const QColor& color, int alpha)
{
    for (int x = rect.left(); x <= rect.right(); ++x)
        for (int y = rect.top(); y <= rect.bottom(); ++y)
            scene->paintCell(QPoint(x, y), QBrush(Qt::transparent));
    QColor translucent = color;
    translucent.setAlpha(alpha);
    for (int x = rect.left(); x <= rect.right(); ++x)
        for (int y = rect.top(); y <= rect.bottom(); ++y)
            scene->paintCell(QPoint(x, y), QBrush(translucent));
}


void MainWindow::clearPolygon()
{
    if (polygonVertices.size() < 2) return;
    polygonPixels.clear();

    for (const QPoint& v : polygonVertices) {
        bool skip = false;
        if (hasClippingWindow) {
            if (clippingWindow.contains(v)) skip = true;
            bool onLeft = (v.x() == clippingWindow.left() && v.y() >= clippingWindow.top() && v.y() <= clippingWindow.bottom());
            bool onRight = (v.x() == clippingWindow.right() && v.y() >= clippingWindow.top() && v.y() <= clippingWindow.bottom());
            bool onTop = (v.y() == clippingWindow.top() && v.x() >= clippingWindow.left() && v.x() <= clippingWindow.right());
            bool onBottom = (v.y() == clippingWindow.bottom() && v.x() >= clippingWindow.left() && v.x() <= clippingWindow.right());
            if (onLeft || onRight || onTop || onBottom) skip = true;
        }
        if (!skip) scene->paintCell(v, QBrush(Qt::transparent));
    }

    for (int i = 0; i < polygonVertices.size(); i++) {
        int nextIndex = (i + 1) % polygonVertices.size();
        QPoint p1 = polygonVertices[i];
        QPoint p2 = polygonVertices[nextIndex];
        int x1 = p1.x(), y1 = p1.y(), x2 = p2.x(), y2 = p2.y();
        int dx = std::abs(x2 - x1), dy = std::abs(y2 - y1);
        int sx = (x1 < x2) ? 1 : -1;
        int sy = (y1 < y2) ? 1 : -1;
        int err = dx - dy;
        while (true) {
            bool skip = false;
            if (hasClippingWindow) {
                if (x1 >= clippingWindow.left() && x1 <= clippingWindow.right() &&
                    y1 >= clippingWindow.top()  && y1 <= clippingWindow.bottom()) skip = true;
                bool onLeft = (x1 == clippingWindow.left() && y1 >= clippingWindow.top() && y1 <= clippingWindow.bottom());
                bool onRight = (x1 == clippingWindow.right() && y1 >= clippingWindow.top() && y1 <= clippingWindow.bottom());
                bool onTop = (y1 == clippingWindow.top() && x1 >= clippingWindow.left() && x1 <= clippingWindow.right());
                bool onBottom = (y1 == clippingWindow.bottom() && x1 >= clippingWindow.left() && x1 <= clippingWindow.right());
                if (onLeft || onRight || onTop || onBottom) skip = true;
            }
            if (!skip) scene->paintCell(QPoint(x1, y1), QBrush(Qt::transparent));
            if (x1 == x2 && y1 == y2) break;
            int e2 = 2 * err;
            if (e2 > -dy) { err -= dy; x1 += sx; }
            if (e2 < dx) { err += dx; y1 += sy; }
        }
    }
}


void MainWindow::clearWindow()
{
    if (hasClippingWindow) {
        for (int x = clippingWindow.left(); x <= clippingWindow.right(); ++x)
            for (int y = clippingWindow.top(); y <= clippingWindow.bottom(); ++y)
                scene->paintCell(QPoint(x, y), QBrush(Qt::transparent));
        drawRectangle(clippingWindow, QBrush(Qt::transparent));
    }
}

bool MainWindow::isInside(const QPointF& point, EdgePosition edge, const QRect& clipRect)
{
    switch (edge) {
    case LEFT_EDGE:
        return point.x() >= clipRect.left();
    case RIGHT_EDGE:
        return point.x() <= clipRect.right();
    case BOTTOM_EDGE:
        return point.y() >= clipRect.top();
    case TOP_EDGE:
        return point.y() <= clipRect.bottom();
    }
    return false;
}

QPointF MainWindow::getIntersection(const QPointF& p1, const QPointF& p2, EdgePosition edge, const QRect& clipRect)
{
    double x1 = p1.x(), y1 = p1.y(), x2 = p2.x(), y2 = p2.y();
    double x = 0, y = 0;

    switch (edge) {

    case LEFT_EDGE:
        x = clipRect.left();
        if (x2 != x1) y = y1 + (y2 - y1) * (x - x1) / (x2 - x1); else y = y1;
        y = std::max<double>(clipRect.top(), std::min<double>(clipRect.bottom(), std::round(y)));
        return QPointF(x, y);

    case RIGHT_EDGE:
        x = clipRect.right();
        if (x2 != x1) y = y1 + (y2 - y1) * (x - x1) / (x2 - x1); else y = y1;
        y = std::max<double>(clipRect.top(), std::min<double>(clipRect.bottom(), std::round(y)));
        return QPointF(x, y);

    case BOTTOM_EDGE:
        y = clipRect.top();
        if (y2 != y1) x = x1 + (x2 - x1) * (y - y1) / (y2 - y1); else x = x1;
        x = std::max<double>(clipRect.left(), std::min<double>(clipRect.right(), std::round(x)));
        return QPointF(x, y);

    case TOP_EDGE:
        y = clipRect.bottom();
        if (y2 != y1) x = x1 + (x2 - x1) * (y - y1) / (y2 - y1); else x = x1;
        x = std::max<double>(clipRect.left(), std::min<double>(clipRect.right(), std::round(x)));
        return QPointF(x, y);
    }
    return QPointF();
}


QList<QPointF> MainWindow::clipAgainstEdge(const QList<QPointF>& inputVertices, EdgePosition edge, const QRect& clipRect)
{
    QList<QPointF> outputVertices;

    if (inputVertices.isEmpty()) {
        return outputVertices;
    }

    QPointF previousVertex = inputVertices.last();

    for (const QPointF& currentVertex : inputVertices) {
        bool currentInside = isInside(currentVertex, edge, clipRect);
        bool previousInside = isInside(previousVertex, edge, clipRect);

        if (currentInside) {
            if (!previousInside) {
                QPointF intersection = getIntersection(previousVertex, currentVertex, edge, clipRect);
                outputVertices.append(intersection);
            }
            outputVertices.append(currentVertex);
        }
        else if (previousInside) {
            QPointF intersection = getIntersection(previousVertex, currentVertex, edge, clipRect);
            outputVertices.append(intersection);
        }

        previousVertex = currentVertex;
    }

    return outputVertices;
}

QList<QPointF> MainWindow::sutherlandHodgemanClip(const QList<QPointF>& polygon, const QRect& clipRect)
{
    QList<QPointF> outputPolygon = polygon;
    outputPolygon = clipAgainstEdge(outputPolygon, LEFT_EDGE, clipRect);

    if (outputPolygon.isEmpty()) return outputPolygon;
    outputPolygon = clipAgainstEdge(outputPolygon, RIGHT_EDGE, clipRect);

    if (outputPolygon.isEmpty()) return outputPolygon;
    outputPolygon = clipAgainstEdge(outputPolygon, BOTTOM_EDGE, clipRect);

    if (outputPolygon.isEmpty()) return outputPolygon;
    outputPolygon = clipAgainstEdge(outputPolygon, TOP_EDGE, clipRect);

    if (outputPolygon.isEmpty()) return outputPolygon;

    QList<QPointF> snapped;
    snapped.reserve(outputPolygon.size());
    for (const QPointF& p : outputPolygon) {
        QPointF q(qRound(p.x()), qRound(p.y()));
        if (snapped.isEmpty() || q != snapped.last()) snapped.append(q);
    }
    if (snapped.size() >= 2 && snapped.first() == snapped.last()) snapped.removeLast();
    return snapped;
}


QPointF MainWindow::computeLineIntersection(const QPointF& p1, const QPointF& p2, const QPointF& p3, const QPointF& p4)
{
    double denom = (p1.x() - p2.x()) * (p3.y() - p4.y()) - (p1.y() - p2.y()) * (p3.x() - p4.x());

    if (qFuzzyIsNull(denom)) {
        return QPointF();
    }

    double t = ((p1.x() - p3.x()) * (p3.y() - p4.y()) - (p1.y() - p3.y()) * (p3.x() - p4.x())) / denom;
    double u = -((p1.x() - p2.x()) * (p1.y() - p3.y()) - (p1.y() - p2.y()) * (p1.x() - p3.x())) / denom;

    if (t >= 0 && t <= 1 && u >= 0 && u <= 1) {
        return QPointF(p1.x() + t * (p2.x() - p1.x()), p1.y() + t * (p2.y() - p1.y()));
    }

    return QPointF();
}


double MainWindow::distance(const QPointF& p1, const QPointF& p2)
{
    return std::sqrt(std::pow(p2.x() - p1.x(), 2) + std::pow(p2.y() - p1.y(), 2));
}


bool MainWindow::isPointInsidePolygon(const QPointF& point, const QList<QPointF>& polygon)
{
    int crossings = 0;
    int n = polygon.size();

    for (int i = 0; i < n; i++) {
        const QPointF& p1 = polygon[i];
        const QPointF& p2 = polygon[(i + 1) % n];

        if (((p1.y() <= point.y() && point.y() < p2.y()) ||
             (p2.y() <= point.y() && point.y() < p1.y())) &&
            point.x() < (p2.x() - p1.x()) * (point.y() - p1.y()) / (p2.y() - p1.y()) + p1.x()) {
            crossings++;
        }
    }

    return (crossings % 2 == 1);
}
