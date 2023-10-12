#ifndef MYGRAPHICSVIEW_H
#define MYGRAPHICSVIEW_H

#include <QGraphicsView>

class MyGraphicsView : public QGraphicsView
{
    Q_OBJECT
public:
    using QGraphicsView::QGraphicsView;
    MyGraphicsView();

protected:
    void mousePressEvent(QMouseEvent* e) override;
signals:
    void mouse_clicked(int x, int y);
};


#endif // MYGRAPHICSVIEW_H
