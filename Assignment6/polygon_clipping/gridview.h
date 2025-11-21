#ifndef GRIDVIEW_H
#define GRIDVIEW_H

#include <QGraphicsView>

class GridScene;

class GridView : public QGraphicsView {
    Q_OBJECT

public:
    explicit GridView(QWidget *parent = nullptr);
    explicit GridView(GridScene *scene, QWidget *parent = nullptr);

    void zoomIn();
    void zoomOut();
    void resetZoom();
    double getZoomFactor() const;

protected:
    void wheelEvent(QWheelEvent *event) override;

private:
    void setZoom(double factor);

    double zoomFactor = 1.0;
    const double zoomIncrement = 0.1;
    const double minZoom = 0.2;
    const double maxZoom = 5.0;
};

#endif // GRIDVIEW_H
