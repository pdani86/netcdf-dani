#include "mygraphicsview.h"

#include <QMouseEvent>
#include <iostream>

MyGraphicsView::MyGraphicsView()
{
//    qRegisterMetaType<QMouseEvent>("QMouseEvent");
}


void MyGraphicsView::mousePressEvent(QMouseEvent* e) {
//    std::cout << "mouse click pos: " << e->pos().x() << ", " << e->pos().y() << std::endl;
    auto pos = mapToScene(e->pos());
    emit mouse_clicked(pos.x(), pos.y());
}
