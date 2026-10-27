#pragma once
#include <QtWidgets>

enum class ToolIcon { Marker, Sponge, Arrow, Rectangle, Text, Glass, Help, Color, Size, Undo, Redo, Camera, Clipboard, Pass, Clear, Close };

class IconButton : public QPushButton {
    ToolIcon icon;
public:
    QColor accent = QColor("#f2cc69");
    IconButton(ToolIcon value, QWidget *parent) : QPushButton(parent), icon(value) {
        setFixedSize(44,44); setCursor(Qt::PointingHandCursor);
    }
protected:
    void paintEvent(QPaintEvent *) override {
        QPainter p(this); p.setRenderHint(QPainter::Antialiasing);
        const QColor fg = !isEnabled()?QColor("#64717b"):isChecked() ? QColor("#202830") : QColor("#e4edf3");
        p.setPen(Qt::NoPen);
        p.setBrush(isChecked()?accent:isDown()?QColor("#52606c"):underMouse()?QColor("#394752"):Qt::transparent);
        p.drawRoundedRect(rect().adjusted(2,2,-2,-2),10,10);
        p.translate(10,10); if(isDown()){p.translate(12,12);p.scale(.96,.96);p.translate(-12,-12);}
        p.setPen(QPen(fg,1.8,Qt::SolidLine,Qt::RoundCap,Qt::RoundJoin)); p.setBrush(Qt::NoBrush);
        switch(icon) {
        case ToolIcon::Marker:
            p.drawPolygon(QPolygonF{{3,17},{14,3},{21,9},{10,22},{3,22}});p.drawLine(11,7,18,13);p.drawLine(3,17,10,22);break;
        case ToolIcon::Sponge:
            p.drawRoundedRect(QRectF(2,5,20,15),4,4);p.drawLine(3,15,21,15);p.drawPoint(7,10);p.drawPoint(13,9);p.drawPoint(18,11);break;
        case ToolIcon::Arrow:p.drawLine(3,21,21,3);p.drawPolyline(QPolygonF{{11,3},{21,3},{21,13}});break;
        case ToolIcon::Rectangle:p.drawRoundedRect(QRectF(3,4,18,16),1,1);break;
        case ToolIcon::Text:p.drawLine(4,4,20,4);p.drawLine(12,4,12,21);p.drawLine(8,21,16,21);break;
        case ToolIcon::Glass:p.setBrush(isChecked()?QColor("#202830"):QColor("#f2f0e8"));p.drawRoundedRect(QRectF(3,3,18,18),3,3);p.setPen(QPen(isChecked()?QColor("#f2f0e8"):QColor("#202830"),1.8));p.drawLine(8,8,16,8);p.drawLine(8,12,16,12);break;
        case ToolIcon::Help:p.drawEllipse(QRectF(2,2,20,20));p.drawText(QRectF(2,1,20,22),Qt::AlignCenter,"?");break;
        case ToolIcon::Redo:
            p.drawPolyline(QPolygonF{{16,4},{21,9},{16,14}});{QPainterPath path;path.moveTo(21,9);path.cubicTo(1,2,-1,23,14,21);p.drawPath(path);}break;
        case ToolIcon::Color:
            p.setBrush(accent);p.setPen(QPen(Qt::white,1.3));p.drawEllipse(QRectF(4,4,16,16));break;
        case ToolIcon::Size:
            p.drawLine(3,6,21,6);p.drawLine(3,12,21,12);p.setPen(QPen(fg,4,Qt::SolidLine,Qt::RoundCap));p.drawLine(3,19,21,19);break;
        case ToolIcon::Undo:
            p.drawPolyline(QPolygonF{{8,4},{3,9},{8,14}}); {QPainterPath path;path.moveTo(3,9);path.cubicTo(23,2,25,23,10,21);p.drawPath(path);}break;
        case ToolIcon::Camera:
            p.drawRoundedRect(QRectF(2,7,20,15),3,3);p.drawPolyline(QPolygonF{{7,7},{9,3},{15,3},{17,7}});p.drawEllipse(QRectF(8,11,8,8));break;
        case ToolIcon::Clipboard:
            p.drawRoundedRect(QRectF(4,4,16,18),2,2);p.setBrush(QColor("#202830"));p.drawRoundedRect(QRectF(8,2,8,5),1.5,1.5);p.setBrush(Qt::NoBrush);p.drawLine(8,12,16,12);p.drawLine(8,17,14,17);break;
        case ToolIcon::Pass:
            p.drawLine(2,12,22,12);p.drawPolyline(QPolygonF{{16,6},{22,12},{16,18}});p.setPen(QPen(fg,1.5,Qt::DashLine));p.drawLine(10,2,10,22);break;
        case ToolIcon::Clear:
            p.drawLine(3,6,21,6);p.drawLine(9,3,15,3);p.drawPolyline(QPolygonF{{5,6},{7,22},{17,22},{19,6}});p.drawLine(10,10,10,18);p.drawLine(14,10,14,18);break;
        case ToolIcon::Close:p.drawLine(6,6,18,18);p.drawLine(18,6,6,18);break;
        }
        if(hasFocus()){p.resetTransform();p.setBrush(Qt::NoBrush);p.setPen(QPen(accent,1,Qt::DotLine));p.drawRoundedRect(rect().adjusted(1,1,-1,-1),11,11);}
    }
};

class DragHandle : public QWidget {
public:
    explicit DragHandle(QWidget *parent):QWidget(parent) {
        setFixedSize(188,36);setCursor(Qt::SizeAllCursor);setToolTip("Перетащите, чтобы переместить доску · Alt + перетаскивание стекла");
    }
protected:
    void mousePressEvent(QMouseEvent *event) override {
        if(event->button()==Qt::LeftButton && window()->windowHandle()) window()->windowHandle()->startSystemMove();
    }
    void paintEvent(QPaintEvent *) override {
        QPainter p(this);p.setRenderHint(QPainter::Antialiasing);p.setPen(QPen(QColor("#63717b"),1));p.setBrush(QColor("#202830"));p.drawRoundedRect(rect().adjusted(1,1,-1,-1),12,12);
        p.setPen(QPen(QColor("#9eafb9"),2.5,Qt::SolidLine,Qt::RoundCap));
        for(int y: {14,20}) for(int x:{14,20,26})p.drawPoint(x,y);
        p.setPen(QColor("#e4edf3"));p.setFont(QFont("Segoe UI",10,QFont::DemiBold));p.drawText(QRect(39,0,135,36),Qt::AlignVCenter,"GLASSBOARD");
    }
};

class BrushPreview : public QWidget {
public:
    int diameter=5; QColor ink=QColor("#f2cc69"); bool erase=false, text=false;
    explicit BrushPreview(QWidget *parent):QWidget(parent){setFixedHeight(66);}
protected:
    void paintEvent(QPaintEvent *) override {
        QPainter p(this);p.setRenderHint(QPainter::Antialiasing);p.setPen(Qt::NoPen);p.setBrush(QColor("#141b21"));p.drawRoundedRect(rect(),8,8);
        if(text){QFont font("Segoe UI");font.setPixelSize(qMin(diameter,48));p.setFont(font);p.setPen(ink);p.drawText(rect().adjusted(8,0,-8,0),Qt::AlignCenter,"Текст Aa");}
        else {p.setPen(QPen(erase?QColor("#9ec5b6"):ink,diameter,Qt::SolidLine,Qt::RoundCap));p.drawLine(40,height()/2,width()-40,height()/2);}
    }
};
