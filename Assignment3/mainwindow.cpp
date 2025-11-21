#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QPixmap>
#include <QImage>
#include <QPainter>
#include <QDebug>
#include <QTimer>
#include <QQueue>
#include <QSet>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    lastPoint1 = QPoint(-100000, -100000);
    lastPoint2 = QPoint(-100000, -100000);

    connect(ui->frame, SIGNAL(Mouse_Pos()), this, SLOT(Mouse_Pressed()));
    connect(ui->frame, SIGNAL(sendMousePosition(QPoint&)), this, SLOT(showMousePosition(QPoint&)));

    gap = 10;
    fellipse = 0;

    width = 750;
    height = 450;

    draw_grid();
    ui->polarellipsetime->setText("Polar: 0 ns");
    ui->bresenhamellipsetime->setText("Bresenham: 0 ns");

    ui->filled->setText("Filled: 0 pixels");

    ui->undo->setEnabled(false);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::showMousePosition(QPoint &pos)
{
    sc_x = pos.x();
    sc_y = pos.y();
    ui->mouse_movement->setText("X : " + QString::number((sc_x-sc_x%gap-((width/2)-(width/2)%gap))/gap) + ", Y : " + QString::number(-(sc_y-sc_y%gap-((height/2)-(height/2)%gap))/gap));
}

void MainWindow::Mouse_Pressed()
{
    org_x = sc_x;
    org_y = sc_y;

    int x = (sc_x-sc_x%gap-((width/2)-(width/2)%gap))/gap;
    int y = (sc_y-sc_y%gap-((height/2)-(height/2)%gap))/gap;

    ui->mouse_pressed->setText("X : " + QString::number(x) + ", Y : " + QString::number(y));

    lastPoint1 = lastPoint2;

    lastPoint2 = QPoint(x, y);
}

void MainWindow::on_clear_clicked()
{
    lastPoint1 = lastPoint2 = QPoint(-100000, -100000);

    colormap.clear();
    draw_grid();

    ui->polarellipsetime->setText("Polar: 0 ns");
    ui->bresenhamellipsetime->setText("Bresenham: 0 ns");

    ui->filled->setText("Filled: 0 pixels");
}

void MainWindow::draw_grid()
{
    QPixmap pix(ui->frame->width(), ui->frame->height());
    pix.fill(QColor(255, 255, 255));
    ui->frame->setPixmap(pix);

    QPixmap pm = ui->frame->pixmap();
    QPainter painter(&pm);
    painter.setPen(QPen(QColor(200, 200, 200)));
    for(int i=0; i<=width; i+=gap) painter.drawLine(QPoint(i,0), QPoint(i, height));
    for(int i=0; i<=height; i+=gap) painter.drawLine(QPoint(0, i), QPoint(width, i));

    painter.end();
    ui->frame->setPixmap(pm);

    paint(draw_line_bresenham(QPoint(-100000, 0), QPoint(100000, 0)), QColor(0, 0, 0));
    paint(draw_line_bresenham(QPoint(0, -100000), QPoint(0, 100000)), QColor(0, 0, 0));
}

void MainWindow::paint(std::vector<QPoint> points, QColor color){
    QPixmap pm = ui->frame->pixmap();
    QPainter painter(&pm);
    for(QPoint p:points){
        int x = p.x()*gap, y = p.y()*gap;
        painter.fillRect(QRect(x+((width/2) - (width/2)%gap), y+((height/2) - (height/2)%gap), gap, gap), color);
    }
    painter.end();
    ui->frame->setPixmap(pm);
    ui->filled->setText("Filled: " + QString::number(points.size()) + " pixels");
}

void MainWindow::paint(QPoint point, QColor color, QPainter& painter){
    int x = point.x()*gap, y = point.y()*gap;
    painter.fillRect(QRect(x+((width/2) - (width/2)%gap), y+((height/2) - (height/2)%gap), gap, gap), color);
}

std::vector<QPoint> MainWindow::draw_line_bresenham(QPoint a, QPoint b) {
    int x1 = a.x(), y1 = a.y();
    int x2 = b.x(), y2 = b.y();

    int dx = abs(x2 - x1);
    int dy = abs(y2 - y1);

    int sx = (x1 < x2) ? 1 : -1;
    int sy = (y1 < y2) ? 1 : -1;

    int err = dx - dy;

    std::vector<QPoint> point;

    while (true) {
        point.push_back(QPoint(x1, y1));

        if (x1 == x2 && y1 == y2) break;

        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x1 += sx; }
        if (e2 < dx)  { err += dx; y1 += sy; }
    }

    return point;
}

void MainWindow::on_spinBox_valueChanged(int value)
{
    gap = value;
    repaint();
}


void MainWindow::repaint(){
    draw_grid();
    QPixmap pm = ui->frame->pixmap();
    QPainter painter(&pm);
    for(const auto& pair : colormap){
        for(QPoint p : pair.first){
            paint(p, pair.second, painter);
        }
    }
    painter.end();
    ui->frame->setPixmap(pm);
}

void MainWindow::on_draw_ellipse_clicked()
{
    if(lastPoint2 == QPoint(-100000, -100000)) return;
    draw_ellipse(lastPoint2, ui->ellipse_a->value(), ui->ellipse_b->value());
}

void MainWindow::draw_ellipse(QPoint centre, int a, int b){
    std::vector<QPoint> point;
    QElapsedTimer timer;
    qint64 time = 0;
    if(fellipse == 0){
        for(int i=0; i<100; i++){
            timer.start();
            point = draw_ellipse_polar(centre, a, b);
            time += timer.nsecsElapsed();
        }
        ui->polarellipsetime->setText("Polar: " + QString::number(time/100) + " ns");
    }
    else if(fellipse == 1){
        for(int i=0; i<100; i++){
            timer.start();
            point = draw_ellipse_bresenham(centre, a, b);
            time += timer.nsecsElapsed();
        }
        ui->bresenhamellipsetime->setText("Bresenham: " + QString::number(time/100) + " ns");
    }
    else{
        point = draw_ellipse_cartesian(centre, a, b);
    }
    colormap.push_back(std::pair(point, fellipse==0? QColor(0, 150, 150) : fellipse==1? QColor(150, 0, 150) : QColor(255, 100, 200)));
    paint(point, fellipse==0? QColor(0, 150, 150) : fellipse==1? QColor(150, 0, 150) : QColor(255, 100, 200));
    ui->undo->setEnabled(true);
}


std::vector<QPoint> MainWindow::draw_ellipse_polar(QPoint centre, int a, int b){
    int cx = centre.x();
    int cy = centre.y();

    std::vector<QPoint> point;

    for(double theta = 0; theta < M_PI/2; theta += 0.5/std::sqrt(a*a*cos(theta)*cos(theta) + b*b*sin(theta)*sin(theta))){
        int x = std::round(a * cos(theta)), y = std::round(b * sin(theta));
        point.push_back(QPoint(cx + x, cy + y));
        point.push_back(QPoint(cx - x, cy + y));
        point.push_back(QPoint(cx + x, cy - y));
        point.push_back(QPoint(cx - x, cy - y));
    }

    return point;
}


std::vector<QPoint> MainWindow::draw_ellipse_cartesian(QPoint centre, int a, int b){
    int cx = centre.x();
    int cy = centre.y();

    std::vector<QPoint> point;
    for(int x = 0; x<=a; x++){
        int y = std::round(b * std::sqrt(1 - (float)(x * x)/(a * a)));
        point.push_back(QPoint(cx + x, cy + y));
        point.push_back(QPoint(cx - x, cy + y));
        point.push_back(QPoint(cx + x, cy - y));
        point.push_back(QPoint(cx - x, cy - y));
    }
    for(int y = 0; y<=b; y++){
        int x = std::round(a * std::sqrt(1 - (float)(y * y)/(b * b)));
        point.push_back(QPoint(cx + x, cy + y));
        point.push_back(QPoint(cx - x, cy + y));
        point.push_back(QPoint(cx + x, cy - y));
        point.push_back(QPoint(cx - x, cy - y));
    }

    return point;
}


std::vector<QPoint> MainWindow::draw_ellipse_bresenham(QPoint centre, int a, int b){
    int cx = centre.x();
    int cy = centre.y();

    std::vector<QPoint> point;

    int x = 0, y = b;
    int d1 = b*b - a*a*b + a*a/4;
    int dx = 2 * b*b * x;
    int dy = 2 * a*a * y;


    while(dx < dy){
        point.push_back(QPoint(cx + x, cy + y));
        point.push_back(QPoint(cx - x, cy + y));
        point.push_back(QPoint(cx + x, cy - y));
        point.push_back(QPoint(cx - x, cy - y));

        if (d1 < 0){
            x = x + 1;
            dx += 2 * b*b;
            d1 += dx + b*b;
        }
        else{
            x++;
            y--;
            dx += 2 * b*b;
            dy -= 2 * a*a;
            d1 += dx - dy + b*b;
        }
    }


    int d2 = b*b * (x+0.5)*(x+0.5) + a*a * (y-1)*(y-1) - a*a * b*b;

    while(y >= 0){
        point.push_back(QPoint(cx + x, cy + y));
        point.push_back(QPoint(cx - x, cy + y));
        point.push_back(QPoint(cx + x, cy - y));
        point.push_back(QPoint(cx - x, cy - y));

        if(d2 > 0){
            y--;
            dy -= 2 * a*a;
            d2 += a*a - dy;
        }
        else{
            y--;
            x++;
            dx += 2 * b*b;
            dy -= 2 * a*a;
            d2 += dx - dy + a*a;
        }
    }

    return point;
}

void MainWindow::on_ellipse_type_currentIndexChanged(int index)
{
    fellipse = index;
}

void MainWindow::on_undo_clicked()
{
    colormap.pop_back();
    if(colormap.empty()) ui->undo->setEnabled(false);
    repaint();
}
