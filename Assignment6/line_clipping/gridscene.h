#ifndef GRIDSCENE_H
#define GRIDSCENE_H

#include <QGraphicsScene>
#include <QMap>
#include <QPoint>
#include <QBrush>
#include <QGraphicsRectItem>

class GridScene : public QGraphicsScene {
    Q_OBJECT

public:
    explicit GridScene(QObject *parent = nullptr);

    void paintCell(const QPoint& cell, const QBrush& brush);
    void clearCells();
    void setCellSize(int size) { cellSize = size; }
    int getCellSize() const { return cellSize; }
    bool isCellFilled(const QPoint& cell) const;

signals:
    void cellClicked(const QPoint& cell);
    void leftClick(const QPoint& cell);
    void rightClick(const QPoint& cell);

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void drawBackground(QPainter* painter, const QRectF& rect) override;

private:
    int cellSize = 10;
    QHash<QPoint, QGraphicsRectItem*> cellItems;
};

#endif // GRIDSCENE_H
