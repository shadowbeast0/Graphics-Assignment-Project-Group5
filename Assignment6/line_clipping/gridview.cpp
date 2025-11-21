#include "gridview.h"
#include "gridscene.h"
#include <QPainter>
#include <QWheelEvent>
#include <QApplication>
#include <QDebug>

GridView::GridView(QWidget *parent)
    : QGraphicsView(parent) {
    setRenderHint(QPainter::Antialiasing, false);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setDragMode(QGraphicsView::NoDrag);
    setMouseTracking(true);
}

GridView::GridView(GridScene *scene, QWidget *parent)
    : QGraphicsView(scene, parent) {
    setRenderHint(QPainter::Antialiasing, false);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setDragMode(QGraphicsView::NoDrag);
    setMouseTracking(true);
}

void GridView::wheelEvent(QWheelEvent *event) {
    if (QApplication::keyboardModifiers() & Qt::ControlModifier) {
        if (event->angleDelta().y() > 0) zoomIn();
        else zoomOut();
        event->accept();
    } else {
        QGraphicsView::wheelEvent(event);
    }
}

void GridView::zoomIn() {
    if (zoomFactor < maxZoom) {
        double newZoom = qMin(zoomFactor + zoomIncrement, maxZoom);
        setZoom(newZoom);
    }
}

void GridView::zoomOut() {
    if (zoomFactor > minZoom) {
        double newZoom = qMax(zoomFactor - zoomIncrement, minZoom);
        setZoom(newZoom);
    }
}

void GridView::resetZoom() {
    setZoom(1.0);
}

double GridView::getZoomFactor() const {
    return zoomFactor;
}

void GridView::setZoom(double factor) {
    if (factor >= minZoom && factor <= maxZoom && factor != zoomFactor) {
        QPointF oldPos = mapToScene(viewport()->rect().center());
        resetTransform();
        scale(factor, factor);
        zoomFactor = factor;
        centerOn(oldPos);
        qDebug() << "Zoom level:" << zoomFactor * 100 << "%";
    }
}
