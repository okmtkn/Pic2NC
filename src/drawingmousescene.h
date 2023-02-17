#ifndef DRAWINGMOUSESCENE_H
#define DRAWINGMOUSESCENE_H

#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsSceneMoveEvent>
#include <QPen>
#include <QTabletEvent>

class DrawingMouseScene : public QGraphicsScene
{
public:
    explicit DrawingMouseScene(QObject *parent = nullptr);
private slots:
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;

public:
    QPen pen_;
private:
    bool penOn_ = false;
    int x0_, y0_;
};

#endif // DRAWINGMOUSESCENE_H
