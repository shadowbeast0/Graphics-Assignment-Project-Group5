#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QPixmap>
#include <QImage>
#include <QPainter>
#include <QDebug>
#include <QVector>
#include <QElapsedTimer>
#include <QInputDialog>
#include <QColorDialog>
#include <cmath>
#include <QQueue>
#include <QStack>
#include <QThread>
#include <QTimer>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    lastPoint1 = QPoint(0, 0);
    lastPoint2 = QPoint(0, 0);
    lastColor = Qt::white;

    // Hardcode the height and width:
    frameSize = 512;
    ui->frame->setFixedSize(frameSize, frameSize);

    connect(ui->frame, SIGNAL(Mouse_Pos()), this, SLOT(Mouse_Pressed()));
    connect(ui->frame, SIGNAL(sendMousePosition(QPoint&)), this, SLOT(showMousePosition(QPoint&)));

    // hardcode the initial gap value:
    gap = 8;
    firstSpinBoxChanged = true;
    ui->spin_box->setValue(8);
    ui->spin_box->setMaximum(frameSize/2);
    ui->spin_box->setMinimum(2);
    firstSpinBoxChanged = false;

    ui->animate_check->setCheckState(Qt::CheckState::Unchecked);

    animate = false;

    drawables.clear();
    int totalCells = frameSize + 2;  // two more for safety
    pixelMatrix = QVector<QVector<QColor>> (totalCells, QVector<QColor> (totalCells, Qt::white));
    for (int i = (frameSize%gap)/2+(((frameSize/gap)%2)?0:(gap/2)); i < totalCells; i++) {
        // For Axes:
        pixelMatrix[i][frameSize / 2] = Qt::black;
        pixelMatrix[frameSize / 2][i] = Qt::black;
    }
    ui->line_dda_timer->setText("---- ns");
    ui->line_bresenham_timer->setText("---- ns");
    ui->bezier_timer->setText("---- ns");
    ui->mouse_pressed->setText("X : --, Y : --");
    ui->mouse_movement->setText("X : --, Y : --");
    drawGrid();
}
MainWindow::~MainWindow()
{
    delete ui;
}

int MainWindow::getBoxX(int pos_x) {
    pos_x -= (frameSize % gap) / 2 + (((frameSize/gap)%2)?0:(-gap/ 2));
    return ((pos_x/gap) + (((pos_x>=0) || ((pos_x%gap)==0))?0:-1)) - ((frameSize/2)/gap);
}
int MainWindow::getBoxY(int pos_y) {
    pos_y -= (frameSize % gap) / 2 + (((frameSize/gap)%2)?0:(-gap/2));
    return ((frameSize / 2) / gap) - ((pos_y / gap) + (((pos_y>=0) || ((pos_y%gap)==0))?0:-1));
}
int MainWindow::getFrameX(int box_x) {
    // Gives the middle x position of the box
    return ((box_x + ((frameSize / 2) / gap)) * gap) + (gap / 2) + (((frameSize/gap)%2)?0:(-gap/2)) + (frameSize % gap) / 2;
}
int MainWindow::getFrameY(int box_y) {
    // Gives the middle y position of the box
    return ((((frameSize / 2) / gap) - box_y) * gap) + (gap / 2) + (((frameSize/gap)%2)?0:(-gap/2)) + (frameSize % gap) / 2;
}
int MainWindow::getMatrixX(int box_x) {
    return (frameSize / 2) + box_x;
}
int MainWindow::getMatrixY(int box_y) {
    return (frameSize / 2) - box_y;
}

void MainWindow::showMousePosition(QPoint &pos)
{
    sc_x = getBoxX(pos.x());
    sc_y = getBoxY(pos.y());
    ui->mouse_movement->setText("X : " + QString::number(sc_x) + ", Y : " + QString::number(sc_y));
}
void MainWindow::Mouse_Pressed()
{
    org_x = sc_x;
    org_y = sc_y;
    ui->mouse_pressed->setText("X : " + QString::number(org_x) + ", Y : " + QString::number(org_y));

    lastPoint1 = lastPoint2;
    lastPoint2 = QPoint(org_x, org_y);
    lastColor = pixelMatrix[getMatrixX(org_x)][getMatrixY(org_y)];
    addPoint(org_x, org_y, Qt::red);
    drawables.append(Drawable({DrawableType::POINT, QPoint(lastPoint2)}));
    pixelMatrix[getMatrixX(org_x)][getMatrixY(org_y)] = Qt::red;

    // Handle Bezier curve control points:
    if (bezier_n1 > 0) {
        bezier_ctrl_pts.append(lastPoint2);
        lastColor = pixelMatrix[getMatrixX(org_x)][getMatrixY(org_y)];
        addPoint(org_x, org_y, QColor(140, 90, 0));
        drawables.append(Drawable({DrawableType::POINT, QPoint(lastPoint2)}));
        pixelMatrix[getMatrixX(org_x)][getMatrixY(org_y)] = QColor(140, 90, 0);
        bezier_n1--;
        if (bezier_n1 == 0) {
            // Bezier control points input complete. Create bezier curve and add drawable object.
            Drawable d;
            d.type = DrawableType::BEZIER_CURVE;
            d.shape = bezier_curve_t({QVector<QPoint>(bezier_ctrl_pts), static_cast<int>(bezier_ctrl_pts.size()) - 1});
            drawables.append(d);
            qint64 time_taken = 0;
            for (int i = 0; i < 8; i++)
                time_taken += (drawBezierCurve(bezier_ctrl_pts, false) >> 3);
            drawBezierCurve(bezier_ctrl_pts, true);
            ui->bezier_timer->setText(QString::number(time_taken) + " ns");
        }
    }
}

void MainWindow::addPoint(const int box_x, const int box_y, const QColor &fillColor)
{
    if ((box_x > frameSize / 2) || (box_x < -frameSize / 2) || (box_y > frameSize / 2) || (box_y < -frameSize / 2)) return;
    QPixmap pm = ui->frame->pixmap();
    QPainter painter(&pm);
    int x = getFrameX(box_x);
    int y = getFrameY(box_y);
    painter.fillRect(QRect(x-(gap/2), y-(gap/2), gap, gap), fillColor);
    painter.end();
    ui->frame->setPixmap(pm);
}
void MainWindow::addPoints(const QVector<QPoint>& points, const QColor& fillColor) {
    if (!animate) {
        // Non-animated: draw all at once
        QPixmap pm = ui->frame->pixmap();
        QPainter painter(&pm);
        for (const QPoint& point : points) {
            if ((point.x() > frameSize / 2) || (point.x() < -frameSize / 2) ||
                (point.y() > frameSize / 2) || (point.y() < -frameSize / 2))
                continue;

            int x  = getFrameX(point.x());
            int y  = getFrameY(point.y());
            int mx = getMatrixX(point.x());
            int my = getMatrixY(point.y());

            if (mx >= 0 && mx <= frameSize && my >= 0 && my <= frameSize)
                pixelMatrix[mx][my] = fillColor;

            painter.fillRect(QRect(x - (gap/2),y - (gap/2), gap, gap), fillColor);
        }
        painter.end();
        ui->frame->setPixmap(pm);

    } else {
        for (const QPoint& point : points) {
            int mx = getMatrixX(point.x());
            int my = getMatrixY(point.y());
            if (mx >= 0 && mx <= frameSize && my >= 0 && my <= frameSize)
                pixelMatrix[mx][my] = fillColor;
        }
        QTimer *timer = new QTimer(this);
        auto idx = std::make_shared<int>(0);
        connect(timer, &QTimer::timeout, this, [this, timer, idx, points, fillColor]() {
            if ((*idx) >= (int)points.size() || (*idx) < 0) {
                timer->stop();
                timer->deleteLater();
            } else {
                addPoint(points[*idx].x(), points[*idx].y(), fillColor);
                (*idx)++;
            }
        });
        timer->start(std::min(100, (int)(3000/points.size())));
    }
}

qint64 MainWindow::drawLineDDA(const QPoint& box0, const QPoint& box1, const bool draw) {
    QVector<QPoint> to_fill;

    int x0 = box0.x(), y0 = box0.y(), x1 = box1.x(), y1 = box1.y();
    int dx = x1 - x0, dy = y1 - y0;
    int steps = std::max(std::abs(dx), std::abs(dy));
    to_fill.reserve(steps + 1);

    QElapsedTimer timer;
    timer.start();  // Start timing only for computation

    if (steps == 0) {
        to_fill.append(box0);
    } else {
        double x_inc = static_cast<double>(dx) / steps;
        double y_inc = static_cast<double>(dy) / steps;
        double x = x0, y = y0;


        for (int i = 0; i <= steps; i++) {
            to_fill.append(QPoint{static_cast<int>(std::round(x)), static_cast<int>(std::round(y))});
            x += x_inc;
            y += y_inc;
        }
    }
    qint64 time = timer.nsecsElapsed();  // Stop timing after computation

    // Perform drawing outside the timed section
    if (draw) addPoints(to_fill, QColor(20, 120, 250));

    return time;
}

qint64 MainWindow::drawLineBresenham(const QPoint& box0, const QPoint& box1, const bool draw) {
    QVector<QPoint> to_fill;

    int x0 = box0.x(), y0 = box0.y(), x1 = box1.x(), y1 = box1.y();
    int dx = std::abs(x1 - x0);
    int dy = std::abs(y1 - y0);
    int max_steps = std::max(dx, dy) + 1;
    to_fill.reserve(max_steps);

    QElapsedTimer timer;
    timer.start();  // Start timing only for computation

    int sx = (x1 >= x0) ? 1 : -1;
    int sy = (y1 >= y0) ? 1 : -1;

    int err = dx - dy;
    int x = x0;
    int y = y0;

    while (true) {
        to_fill.append(QPoint{x, y});

        if (x == x1 && y == y1)
            break;

        int e2 = 2 * err;

        if (e2 > -dy) {
            err -= dy;
            x += sx;
        }
        if (e2 < dx) {
            err += dx;
            y += sy;
        }
    }

    qint64 time = timer.nsecsElapsed();  // Stop timing after computation

    // Perform drawing outside the timed section
    if (draw) addPoints(to_fill, QColor(30, 30, 100));

    return time;
}

long long choose(int n, int k) {
    if (k < 0 || k > n) return 0;
    if (k == 0 || k == n) return 1;

    if (k > n / 2) k = n - k;

    long long res = 1;
    for (int i = 1; i <= k; ++i)
        res = res * (n - i + 1) / i;
    return res;
}
qint64 MainWindow::drawBezierCurve(const QVector<QPoint>& control_points, bool draw) {
    int n = control_points.size() - 1, prev_x, prev_y;
    bool first_it = true;
    double u = 0, step = 1.0 / 730.0;
    QVector<QPoint> to_fill;
    to_fill.reserve(static_cast<int>(0.5 + (1.0 / step)));

    QElapsedTimer timer;
    timer.start();  // Start timing only for computation

    while (u <= 1) {
        double xf = 0, yf = 0;
        for (int k = 0; k <= n; k++) {
            xf += pow(u, k) * pow(1 - u, n - k) * control_points[k].x() * choose(n, k);
            yf += pow(u, k) * pow(1 - u, n - k) * control_points[k].y() * choose(n, k);
        }
        int x = static_cast<int>(0.5 + xf), y = static_cast<int>(0.5 + yf);
        if ((!first_it) && (x == prev_x) && (y == prev_y)) {
            u += step;
            continue;
        }
        if (first_it) first_it = false;
        prev_x = x;
        prev_y = y;
        to_fill.append({x, y});
        u += step;
    }
    qint64 time = timer.nsecsElapsed();  // Stop timing after computation

    if (draw) addPoints(to_fill, QColor(200, 150, 20));

    return time;
}

void MainWindow::drawGrid()
{
    QPixmap pix(ui->frame->width(), ui->frame->height());
    pix.fill(Qt::white);
    ui->frame->setPixmap(pix);

    QPixmap pm = ui->frame->pixmap();
    QPainter painter(&pm);

    // Draw gridlines:
    painter.setPen(QPen(QColor(200, 200, 200)));
    for (int i=(frameSize%gap)/2+(((frameSize/gap)%2)?0:(gap/2)); i<=frameSize; i+=gap) {
        painter.drawLine(QPoint(i,0), QPoint(i, frameSize));
        painter.drawLine(QPoint(0, i), QPoint(frameSize, i));
    }

    // Draw the pixelMatrix:
    int x, y;
    for (int i=(frameSize%gap)/2+(((frameSize/gap)%2)?0:(gap/2)) - gap; i <= frameSize; i += gap) {
        x = getMatrixX(getBoxX(i+(gap/2)));
        for (int j=(frameSize%gap)/2+(((frameSize/gap)%2)?0:(gap/2)) - gap; j <= frameSize; j += gap) {
            y = getMatrixY(getBoxY(j+(gap/2)));
            if ((x >= 0) && (x < pixelMatrix.size()) && (y >= 0) && (y < pixelMatrix[0].size())) {
                if (pixelMatrix[x][y] != Qt::white)
                    painter.fillRect(QRect(i, j, gap, gap), pixelMatrix[x][y]);
            }
        }
    }

    painter.end();
    ui->frame->setPixmap(pm);
}

void MainWindow::on_clear_clicked()
{
    lastPoint1 = lastPoint2 = QPoint(0, 0);
    drawables.clear();
    int totalCells = frameSize + 2;  // two more for safety
    pixelMatrix = QVector<QVector<QColor>> (totalCells, QVector<QColor> (totalCells, Qt::white));
    for (int i = 0; i < totalCells; i++) {
        // For Axes:
        pixelMatrix[i][frameSize / 2] = Qt::black;
        pixelMatrix[frameSize / 2][i] = Qt::black;
    }
    ui->line_dda_timer->setText("---- ns");
    ui->line_bresenham_timer->setText("---- ns");
    ui->bezier_timer->setText("---- ns");
    ui->mouse_pressed->setText("X : --, Y : --");
    ui->mouse_movement->setText("X : --, Y : --");
    drawGrid();
}

void MainWindow::on_spin_box_valueChanged(int arg1)
{
    if (!firstSpinBoxChanged && arg1 > 1 && arg1 <= frameSize/2) {
        gap = arg1;
        drawGrid();
    }
}
void MainWindow::on_spin_box_textChanged(const QString &arg1) {}

void MainWindow::on_draw_line_dda_clicked()
{
    Drawable d;
    d.type = DrawableType::LINE_DDA;
    d.shape = line_t({lastPoint1, lastPoint2});
    drawables.append(d);
    qint64 time_taken = 0;
    for (int i = 0; i < 8; ++i)
        time_taken += (drawLineDDA(lastPoint1, lastPoint2, false) >> 3);
    drawLineDDA(lastPoint1, lastPoint2, true);
    ui->line_dda_timer->setText(QString::number(time_taken) + " ns");
}

void MainWindow::on_draw_line_bresenham_clicked()
{
    Drawable d;
    d.type = DrawableType::LINE_BSH;
    d.shape = line_t({lastPoint1, lastPoint2});
    drawables.append(d);
    qint64 time_taken = 0;
    for (int i = 0; i < 8; ++i)
        time_taken += (drawLineBresenham(lastPoint1, lastPoint2, false) >> 3);
    drawLineBresenham(lastPoint1, lastPoint2, true);
    ui->line_bresenham_timer->setText(QString::number(time_taken) + " ns");
}

void MainWindow::on_animate_check_checkStateChanged(const Qt::CheckState &arg1)
{
    if (arg1 == Qt::CheckState::Checked) animate = true;
    else animate = false;
}

void MainWindow::on_draw_bezier_clicked()
{
    bool ok;
    int s = QInputDialog::getInt(this,
                                 tr("Enter Number of Control Points:"),  // Dialog title
                                 tr("N + 1:"),                   // Label
                                 4,                          // Default value
                                 3,                          // Minimum allowed
                                 16,                         // Maximum allowed
                                 1,                          // Step increment
                                 &ok);                       // Output parameter for OK/Cancel
    bezier_n1 = (s >= 3 && s <= 10) ? s : 0;
    bezier_ctrl_pts = QVector<QPoint>();
    bezier_ctrl_pts.reserve(bezier_n1);
}

