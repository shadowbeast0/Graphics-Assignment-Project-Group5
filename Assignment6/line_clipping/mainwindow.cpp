#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "gridscene.h"
#include "gridview.h"
#include <QMessageBox>
#include <cmath>
#include <algorithm>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , scene(new GridScene(this))
    , isDrawingLine(false)
    , lineClickCount(0)
    , hasClippingWindow(false)
    , isDrawingWindow(false)
    , windowClickCount(0)
{
    ui->setupUi(this);
    ui->grid->setScene(scene);
    ui->grid->setFixedSize(ui->grid->width(), ui->grid->height());
    ui->grid->centerOn(0, 0);
    scene->setCellSize(10);

    connect(ui->drawLine, &QPushButton::clicked, this, &MainWindow::onDrawLine);
    connect(ui->restoreLine, &QPushButton::clicked, this, &MainWindow::onRestoreLine);
    connect(ui->eraseLine, &QPushButton::clicked, this, &MainWindow::onEraseLine);
    connect(ui->drawWindow, &QPushButton::clicked, this, &MainWindow::onDrawClippingWindow);
    connect(ui->eraseWindow, &QPushButton::clicked, this, &MainWindow::onEraseClippingWindow);
    connect(ui->clipLineCohenSuther, &QPushButton::clicked, this, &MainWindow::onClipLineCohenSutherland);
    connect(ui->clipLineLiangBarsky, &QPushButton::clicked, this, &MainWindow::onClipLineLiangBarsky);
    connect(ui->clearAll, &QPushButton::clicked, this, &MainWindow::onClearAll);
    connect(scene, &GridScene::leftClick, this, &MainWindow::onCellClicked);
}

MainWindow::~MainWindow()
{
    delete scene;
    delete ui;
}

void MainWindow::onCellClicked(const QPoint& cell)
{
    if (isDrawingLine) {
        linePoints.append(cell);
        lineClickCount++;
        scene->paintCell(cell, QBrush(Qt::blue));

        if (lineClickCount == 2) {
            bresenhamLine(linePoints[0], linePoints[1], QBrush(Qt::blue));
            originalLinePoints = linePoints;
            isDrawingLine = false;
            lineClickCount = 0;
            scene->update();
        }
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

            hasClippingWindow = true;
            isDrawingWindow = false;
            windowClickCount = 0;
            scene->update();
        }
    }
}

void MainWindow::fillWindow(const QRect& rect, const QColor& color, int alpha)
{
    QColor translucent = color;
    translucent.setAlpha(alpha);
    for (int x = rect.left(); x <= rect.right(); ++x)
        for (int y = rect.top(); y <= rect.bottom(); ++y) {
            if (!scene->isCellFilled(QPoint(x, y)))  // skip if already painted
                scene->paintCell(QPoint(x, y), QBrush(translucent));
        }
}

void MainWindow::onDrawLine()
{
    if (isDrawingWindow) {
        QMessageBox::warning(this, "Warning", "Please finish drawing the clipping window first!");
        return;
    }

    clearLine();
    linePoints.clear();
    originalLinePoints.clear();
    isDrawingLine = true;
    lineClickCount = 0;
    QMessageBox::information(this, "Draw Line", "Click two points to draw a line.");
}

void MainWindow::onRestoreLine()
{
    if (originalLinePoints.size() != 2) {
        QMessageBox::warning(this, "Warning", "No original line to restore!");
        return;
    }

    clearLine();
    linePoints = originalLinePoints;

    if (hasClippingWindow) {
        fillWindow(clippingWindow, QColor(173, 216, 230), 120);
        drawRectangle(clippingWindow, QBrush(Qt::red), 1);
    }

    bresenhamLine(linePoints[0], linePoints[1], QBrush(Qt::blue));
    scene->update();
}

void MainWindow::onEraseLine()
{
    clearLine();
    linePoints.clear();
    originalLinePoints.clear();
    scene->update();
}

void MainWindow::onDrawClippingWindow()
{
    if (isDrawingLine) {
        QMessageBox::warning(this, "Warning", "Please finish drawing the line first!");
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

void MainWindow::onClipLineCohenSutherland()
{
    if (linePoints.size() != 2) {
        QMessageBox::warning(this, "Warning", "Please draw a line first!");
        return;
    }

    if (!hasClippingWindow) {
        QMessageBox::warning(this, "Warning", "Please draw a clipping window first!");
        return;
    }

    clearLine();
    fillWindow(clippingWindow, QColor(173, 216, 230), 120);
    drawRectangle(clippingWindow, QBrush(Qt::red), 1);

    drawPartialLine(linePoints[0], linePoints[1], clippingWindow, QBrush(Qt::green), QBrush(Qt::gray), true);

    scene->update();
}

void MainWindow::onClipLineLiangBarsky()
{
    if (linePoints.size() != 2) {
        QMessageBox::warning(this, "Warning", "Please draw a line first!");
        return;
    }

    if (!hasClippingWindow) {
        QMessageBox::warning(this, "Warning", "Please draw a clipping window first!");
        return;
    }

    clearLine();
    fillWindow(clippingWindow, QColor(173, 216, 230), 120);
    drawRectangle(clippingWindow, QBrush(Qt::red), 1);

    drawPartialLine(linePoints[0], linePoints[1], clippingWindow, QBrush(Qt::green), QBrush(Qt::gray), false);

    scene->update();
}

void MainWindow::onClearAll()
{
    scene->clearCells();
    linePoints.clear();
    originalLinePoints.clear();
    isDrawingLine = false;
    lineClickCount = 0;
    hasClippingWindow = false;
    isDrawingWindow = false;
    windowClickCount = 0;
    scene->update();
}

void MainWindow::bresenhamLine(const QPoint& p1, const QPoint& p2, const QBrush& brush)
{
    int x1 = p1.x(), y1 = p1.y(), x2 = p2.x(), y2 = p2.y();
    int dx = abs(x2 - x1), dy = abs(y2 - y1);
    int sx = (x1 < x2) ? 1 : -1;
    int sy = (y1 < y2) ? 1 : -1;
    int err = dx - dy;

    while (true) {
        scene->paintCell(QPoint(x1, y1), brush);
        if (x1 == x2 && y1 == y2) break;
        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x1 += sx; }
        if (e2 < dx) { err += dx; y1 += sy; }
    }
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

bool MainWindow::isPointInsideWindow(const QPoint& p, const QRect& window)
{
    return p.x() >= window.left() && p.x() <= window.right() && p.y() >= window.top() && p.y() <= window.bottom();
}

void MainWindow::drawPartialLine(const QPoint& p1, const QPoint& p2, const QRect& window, const QBrush& insideBrush, const QBrush& outsideBrush, bool useCohenSutherland)
{
    int x1 = p1.x(), y1 = p1.y(), x2 = p2.x(), y2 = p2.y();
    int dx = abs(x2 - x1), dy = abs(y2 - y1);
    int sx = (x1 < x2) ? 1 : -1;
    int sy = (y1 < y2) ? 1 : -1;
    int err = dx - dy;

    while (true) {
        QPoint cell(x1, y1);
        if (isPointInsideWindow(cell, window))
            scene->paintCell(cell, insideBrush);
        else
            scene->paintCell(cell, outsideBrush);

        if (x1 == x2 && y1 == y2) break;
        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x1 += sx; }
        if (e2 < dx)  { err += dx; y1 += sy; }
    }
}


void MainWindow::clearLine()
{
    if (linePoints.size() != 2) return;

    int x1 = linePoints[0].x(), y1 = linePoints[0].y();
    int x2 = linePoints[1].x(), y2 = linePoints[1].y();
    int dx = abs(x2 - x1), dy = abs(y2 - y1);
    int sx = (x1 < x2) ? 1 : -1;
    int sy = (y1 < y2) ? 1 : -1;
    int err = dx - dy;

    while (true) {
        QPoint cell(x1, y1);
        bool isOnWindow = hasClippingWindow && isPointInsideWindow(cell, clippingWindow);
        if (!isOnWindow)
            scene->paintCell(cell, QBrush(Qt::transparent));

        if (x1 == x2 && y1 == y2) break;
        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x1 += sx; }
        if (e2 < dx) { err += dx; y1 += sy; }
    }
}

void MainWindow::clearWindow()
{
    if (hasClippingWindow) {
        for (int x = clippingWindow.left(); x <= clippingWindow.right(); ++x)
            for (int y = clippingWindow.top(); y <= clippingWindow.bottom(); ++y)
                scene->paintCell(QPoint(x, y), QBrush(Qt::transparent));
    }
}

int MainWindow::computeOutCode(double x, double y, const QRect& rect)
{
    int code = INSIDE;
    if (x < rect.left()) code |= LEFT;
    else if (x > rect.right()) code |= RIGHT;
    if (y < rect.top()) code |= BOTTOM;
    else if (y > rect.bottom()) code |= TOP;
    return code;
}

bool MainWindow::cohenSutherlandClip(double& x1, double& y1, double& x2, double& y2, const QRect& rect)
{
    int outcode1 = computeOutCode(x1, y1, rect);
    int outcode2 = computeOutCode(x2, y2, rect);
    bool accept = false;

    while (true) {
        if (!(outcode1 | outcode2)) { accept = true; break; }
        else if (outcode1 & outcode2) break;
        else {
            int outcodeOut = outcode1 ? outcode1 : outcode2;
            double x, y;

            if (outcodeOut & TOP) {
                x = x1 + (x2 - x1) * (rect.bottom() - y1) / (y2 - y1);
                y = rect.bottom();
            }
            else if (outcodeOut & BOTTOM) {
                x = x1 + (x2 - x1) * (rect.top() - y1) / (y2 - y1);
                y = rect.top();
            }
            else if (outcodeOut & RIGHT) {
                y = y1 + (y2 - y1) * (rect.right() - x1) / (x2 - x1);
                x = rect.right();
            }
            else if (outcodeOut & LEFT) {
                y = y1 + (y2 - y1) * (rect.left() - x1) / (x2 - x1);
                x = rect.left();
            }

            if (outcodeOut == outcode1) {
                x1 = x; y1 = y;
                outcode1 = computeOutCode(x1, y1, rect);
            }
            else {
                x2 = x; y2 = y;
                outcode2 = computeOutCode(x2, y2, rect);
            }
        }
    }
    return accept;
}

bool MainWindow::liangBarskyClip(double& x1, double& y1, double& x2, double& y2, const QRect& rect)
{
    double xmin = rect.left(), xmax = rect.right(), ymin = rect.top(), ymax = rect.bottom();
    double dx = x2 - x1, dy = y2 - y1, t0 = 0.0, t1 = 1.0;
    double p[4] = {-dx, dx, -dy, dy};
    double q[4] = {x1 - xmin, xmax - x1, y1 - ymin, ymax - y1};

    for (int i = 0; i < 4; i++) {
        if (p[i] == 0 && q[i] < 0) return false;
        double t = q[i] / p[i];
        if (p[i] < 0) t0 = std::max(t0, t);
        else if (p[i] > 0) t1 = std::min(t1, t);
        if (t0 > t1) return false;
    }

    if (t0 > 0) { x1 += t0 * dx; y1 += t0 * dy; }
    if (t1 < 1) { x2 -= (1 - t1) * dx; y2 -= (1 - t1) * dy; }

    return true;
}
