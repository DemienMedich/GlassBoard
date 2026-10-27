#pragma once
#include <QtWidgets>
#include <cmath>

enum class DrawingTool { Marker, Eraser, Arrow, Rectangle, Text };
enum class StrokeKind { Freehand, Arrow, Rectangle, Text };
struct Stroke {
    QPolygonF points;
    QColor color;
    qreal width;
    bool erase;
    StrokeKind kind = StrokeKind::Freehand;
    QString text;
};

inline QPointF constrainedEnd(QPointF start, QPointF end, bool square) {
    const QPointF d=end-start;
    if(square) { const qreal side=qMax(qAbs(d.x()),qAbs(d.y()));return start+QPointF(d.x()<0?-side:side,d.y()<0?-side:side); }
    constexpr qreal quarterPi=0.7853981633974483;
    const qreal angle=std::round(std::atan2(d.y(),d.x())/quarterPi)*quarterPi;
    const qreal length=std::hypot(d.x(),d.y());return start+QPointF(std::cos(angle)*length,std::sin(angle)*length);
}

inline void paintStroke(QPainter &p, const Stroke &s) {
    if(s.points.isEmpty()) return;
    p.setCompositionMode(s.erase?QPainter::CompositionMode_Clear:QPainter::CompositionMode_SourceOver);
    p.setPen(QPen(s.color,s.width,Qt::SolidLine,Qt::RoundCap,Qt::RoundJoin));p.setBrush(Qt::NoBrush);
    if(s.kind==StrokeKind::Text) {
        QFont font("Segoe UI");font.setPixelSize(qRound(s.width));p.setFont(font);p.setPen(s.color);
        // Lay out text in document coordinates so screen and export use the same wrapping.
        QString text=s.text;text.replace('\n',QChar::LineSeparator);
        QTextLayout layout(text,font);QTextOption opt;opt.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);layout.setTextOption(opt);
        layout.beginLayout();qreal y=0;const qreal width=s.points.size()>1?s.points[1].x()-s.points[0].x():300;
        while(true){auto line=layout.createLine();if(!line.isValid())break;line.setLineWidth(qMax(1.0,width));line.setPosition(QPointF(0,y));y+=line.height();}
        layout.endLayout();layout.draw(&p,s.points[0]);return;
    }
    if(s.kind==StrokeKind::Rectangle && s.points.size()>1){p.drawRect(QRectF(s.points[0],s.points[1]).normalized());return;}
    if(s.kind==StrokeKind::Arrow && s.points.size()>1){
        const QPointF a=s.points[0],b=s.points[1];const qreal length=QLineF(a,b).length();if(length<.5)return;
        p.drawLine(a,b);const QPointF direction=(b-a)/length,normal(-direction.y(),direction.x());
        const qreal head=qMin(length*.45,qMax(12.0,s.width*3.0));
        p.setPen(Qt::NoPen);p.setBrush(s.color);p.drawPolygon(QPolygonF{b,b-direction*head+normal*head*.48,b-direction*head-normal*head*.48});return;
    }
    if(s.points.size()==1)p.drawPoint(s.points.first());else p.drawPolyline(s.points);
}
