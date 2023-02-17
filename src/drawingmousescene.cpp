#include "drawingmousescene.h"

DrawingMouseScene::DrawingMouseScene(QObject *parent)
    : QGraphicsScene{parent}
{
    setSceneRect(-10,-10,20,20);
    pen_ = QPen(Qt::black, 20, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);

}


void DrawingMouseScene::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    penOn_ = true;
    x0_ = event->scenePos().x();
    y0_ = event->scenePos().y();
}

void DrawingMouseScene::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    penOn_ = false;
}

void DrawingMouseScene::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if(penOn_){
        const int x1 = event->scenePos().x();
        const int y1 = event->scenePos().y();
        addLine(x0_,y0_,x1,y1,pen_);
        x0_ = x1;
        y0_ = y1;
    }
}
