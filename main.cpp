#include <QtWidgets>
#include <windows.h>

struct Stroke { QPolygonF points; QColor color; qreal width; bool erase; };

class Board : public QWidget {
    QVector<Stroke> strokes, redoStack;
    QWidget *panel;
    QLabel *status;
    QPushButton *marker, *eraser, *passButton;
    QColor ink = QColor("#ffcf56");
    int thickness = 5, eraserSize = 35;
    QSlider *sizeSlider;
    QLabel *sizeLabel;
    int toolSize() const { return erasing ? eraserSize : thickness; }
    bool erasing = false, drawing = false, passing = false, hotkey = false;
    QCursor markerCursor, spongeCursor;
    void refreshCursors() {
        const qreal dpr = devicePixelRatioF();
        QPixmap pen(QSize(48,48) * dpr); pen.setDevicePixelRatio(dpr); pen.fill(Qt::transparent);
        { QPainter p(&pen); p.setRenderHint(QPainter::Antialiasing);
          p.setPen(QPen(Qt::white, 4)); p.drawLine(QPointF(9,38), QPointF(34,10));
          p.setPen(QPen(QColor("#17212b"), 2));
          QPolygonF body{QPointF(12,29),QPointF(27,8),QPointF(38,16),QPointF(22,37)};
          p.setBrush(QColor("#e5ebef")); p.drawPolygon(body);
          p.setBrush(ink); p.drawPolygon(QPolygonF{QPointF(24,12),QPointF(27,8),QPointF(38,16),QPointF(35,20)});
          p.setBrush(QColor("#28343e")); p.drawPolygon(QPolygonF{QPointF(12,29),QPointF(22,37),QPointF(12,40),QPointF(8,37)});
          p.setPen(QPen(Qt::white,3)); p.drawPoint(QPointF(8,40));
          p.setPen(QPen(ink,2)); p.drawPoint(QPointF(8,40));
        }
        markerCursor=QCursor(pen,8,40);
        const int side=qMax(48,eraserSize+8); const qreal center=side/2.0;
        QPixmap sponge(QSize(side,side)*dpr); sponge.setDevicePixelRatio(dpr); sponge.fill(Qt::transparent);
        { QPainter p(&sponge); p.setRenderHint(QPainter::Antialiasing);
          const qreal radius=eraserSize/2.0;
          p.setBrush(Qt::NoBrush); p.setPen(QPen(QColor(255,255,255,230),3)); p.drawEllipse(QPointF(center,center),radius,radius);
          p.setPen(QPen(QColor(25,35,45,230),1,Qt::DashLine)); p.drawEllipse(QPointF(center,center),radius,radius);
          p.translate(center,center); p.rotate(-24);
          p.setPen(QPen(QColor("#24323e"),1.5)); p.setBrush(QColor("#efc56b")); p.drawRoundedRect(QRectF(-13,-9,26,18),4,4);
          p.setBrush(QColor("#78ad9c")); p.drawRoundedRect(QRectF(-13,3,26,7),2,2);
          p.setPen(QPen(QColor("#bb8e40"),2)); p.drawPoint(-7,-4); p.drawPoint(2,-3); p.drawPoint(8,-5);
        }
        spongeCursor=QCursor(sponge,side/2,side/2);
        updateCursorAt(mapFromGlobal(QCursor::pos()));
    }
    void updateCursorAt(QPoint pt) {
        bool l=pt.x()<8, r=pt.x()>width()-9, t=pt.y()<8, b=pt.y()>height()-9;
        if ((l&&t)||(r&&b)) setCursor(Qt::SizeFDiagCursor);
        else if ((r&&t)||(l&&b)) setCursor(Qt::SizeBDiagCursor);
        else if(l||r) setCursor(Qt::SizeHorCursor);
        else if(t||b) setCursor(Qt::SizeVerCursor);
        else if(canvas().contains(pt)) setCursor(erasing?spongeCursor:markerCursor);
        else setCursor(Qt::SizeAllCursor);
    }
    QRect canvas() const { return rect().adjusted(8, 8, -80, -8); }
    void renderInk(QPainter &p) const {
        for (const auto &s : strokes) {
            p.setCompositionMode(s.erase ? QPainter::CompositionMode_Clear : QPainter::CompositionMode_SourceOver);
            p.setPen(QPen(s.color, s.width, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
            if (s.points.size() == 1) p.drawPoint(s.points.first());
            else p.drawPolyline(s.points);
        }
    }
    QImage inkImage() const {
        QImage img(size() * devicePixelRatioF(), QImage::Format_ARGB32_Premultiplied);
        img.setDevicePixelRatio(devicePixelRatioF()); img.fill(Qt::transparent);
        QPainter p(&img); p.setRenderHint(QPainter::Antialiasing); p.setClipRect(canvas()); renderInk(p); return img;
    }
    QPushButton *button(QString text, QString tip, std::function<void()> action, bool check = false) {
        auto *b = new QPushButton(text, panel); b->setFixedSize(48, 40);
        b->setToolTip(tip); b->setCheckable(check); connect(b, &QPushButton::clicked, this, action); return b;
    }
    void tool(bool erase) {
        erasing = erase; marker->setChecked(!erase); eraser->setChecked(erase);
        const QSignalBlocker blocker(sizeSlider); sizeSlider->setValue(toolSize());
        sizeLabel->setText(QString::number(toolSize()) + " px");
        sizeSlider->setToolTip(erasing ? "Размер губки: 1–50 px" : "Размер маркера: 1–50 px");
        refreshCursors(); update();
    }
    void undo() { if (!strokes.isEmpty()) { redoStack.push_back(strokes.takeLast()); update(); } }
    void redo() { if (!redoStack.isEmpty()) { strokes.push_back(redoStack.takeLast()); update(); } }
    void pass() {
        if (!hotkey) { QMessageBox::information(this, "GlassBoard", "Ctrl+Alt+G занята другой программой. Режим пропуска кликов недоступен."); return; }
        passing = !passing;
        auto hwnd = reinterpret_cast<HWND>(winId());
        auto ex = GetWindowLongPtr(hwnd, GWL_EXSTYLE);
        SetWindowLongPtr(hwnd, GWL_EXSTYLE, passing ? ex | WS_EX_TRANSPARENT : ex & ~WS_EX_TRANSPARENT);
        passButton->setChecked(passing); status->setText(passing ? "СКВОЗЬ" : "СТЕКЛО"); update();
    }
    void screenshot(std::function<void(const QImage &)> completed = {}) {
        const QRect area(mapToGlobal(canvas().topLeft()), canvas().size());
        const QImage overlay = inkImage().copy(QRect(canvas().topLeft() * devicePixelRatioF(), canvas().size() * devicePixelRatioF()));
        hide();
        QTimer::singleShot(220, this, [this, area, overlay, completed] {
            QImage shot(area.size() * devicePixelRatioF(), QImage::Format_ARGB32_Premultiplied);
            shot.setDevicePixelRatio(devicePixelRatioF()); shot.fill(Qt::black);
            { QPainter p(&shot);
              for (auto *screen : QGuiApplication::screens()) {
                  const QRect part = area.intersected(screen->geometry());
                  if (part.isEmpty()) continue;
                  const QPoint pos = part.topLeft() - screen->geometry().topLeft();
                  const QPixmap pixels = screen->grabWindow(0, pos.x(), pos.y(), part.width(), part.height());
                  p.drawPixmap(QRect(part.topLeft() - area.topLeft(), part.size()), pixels);
              }
              p.drawImage(QPoint(0, 0), overlay);
            }
            QApplication::clipboard()->setImage(shot);
            show();
            if (completed) { completed(shot); return; }
            const auto path = QFileDialog::getSaveFileName(this, "Снимок уже скопирован — сохранить PNG", QStandardPaths::writableLocation(QStandardPaths::PicturesLocation) + "/GlassBoard_" + QDateTime::currentDateTime().toString("yyyy-MM-dd_HH-mm-ss") + ".png", "PNG (*.png)");
            if (!path.isEmpty() && !shot.save(path)) QMessageBox::warning(this, "GlassBoard", "Не удалось сохранить изображение.");
        });
    }
protected:
    bool nativeEvent(const QByteArray &, void *message, qintptr *result) override {
        auto *msg = static_cast<MSG *>(message);
        if (msg->message == WM_HOTKEY && msg->wParam == 1) { pass(); *result = 0; return true; }
        return false;
    }
    void resizeEvent(QResizeEvent *) override { panel->move(width() - 70, (height() - panel->height()) / 2); update(); }
    void paintEvent(QPaintEvent *) override {
        QPainter p(this); p.setRenderHint(QPainter::Antialiasing);
        p.setPen(Qt::NoPen); p.setBrush(QColor(210, 233, 242, passing ? 3 : 12)); p.drawRoundedRect(rect().adjusted(6,6,-6,-6), 12,12);
        QLinearGradient frame(0,0,width(),height()); frame.setColorAt(0, QColor(228,235,239,225)); frame.setColorAt(.5, QColor(97,111,122,220)); frame.setColorAt(1,QColor(199,211,220,235));
        p.setBrush(Qt::NoBrush); p.setPen(QPen(frame, 4)); p.drawRoundedRect(rect().adjusted(4,4,-4,-4), 14,14);
        p.setPen(QPen(QColor(255,255,255,150),1)); p.drawRoundedRect(rect().adjusted(7,7,-7,-7),10,10);
        p.drawImage(0,0,inkImage());
        p.setPen(QPen(QColor(228,236,242,180),2));
        for(int i=0;i<3;++i) p.drawLine(width()-14-i*4,height()-10,width()-10,height()-14-i*4);
    }
    void mousePressEvent(QMouseEvent *e) override {
        if (e->button() != Qt::LeftButton) return;
        const QPoint pt = e->position().toPoint(); Qt::Edges edges;
        if (pt.x()<8) edges |= Qt::LeftEdge; if(pt.x()>width()-9) edges |= Qt::RightEdge;
        if (pt.y()<8) edges |= Qt::TopEdge; if(pt.y()>height()-9) edges |= Qt::BottomEdge;
        if(edges) { windowHandle()->startSystemResize(edges); return; }
        if (!canvas().contains(pt) || e->modifiers().testFlag(Qt::AltModifier)) { windowHandle()->startSystemMove(); return; }
        redoStack.clear(); strokes.push_back({QPolygonF{e->position()},ink,qreal(toolSize()),erasing}); drawing=true; update();
    }
    void mouseMoveEvent(QMouseEvent *e) override {
        if(drawing) { strokes.last().points << e->position(); update(); return; }
        updateCursorAt(e->position().toPoint());
    }
    void mouseReleaseEvent(QMouseEvent *) override { drawing=false; }
    void closeEvent(QCloseEvent *) override { QSettings s; s.setValue("geometry", saveGeometry()); s.setValue("color",ink); s.setValue("width",thickness); s.setValue("eraserSize",eraserSize); }
public:
    Board() {
        setWindowTitle("GlassBoard " + QCoreApplication::applicationVersion()); setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Window);
        setAttribute(Qt::WA_TranslucentBackground); setMouseTracking(true); setMinimumSize(260,540); resize(1000,680);
        panel = new QWidget(this); panel->setObjectName("tools"); panel->setFixedSize(60,492);
        panel->setStyleSheet("QWidget#tools { background:#202830; border:1px solid #64717b; border-radius:16px; } QPushButton { color:#e4edf3; background:transparent; border:0; border-radius:9px; font:18px 'Segoe UI'; } QPushButton:hover { background:#3c4955; } QPushButton:checked { background:#f2cc69; color:#202830; } QLabel {color:#acbac4; font:8px 'Segoe UI';} QComboBox {background:#36424d;color:white;border:0;border-radius:5px;padding:3px;} ");
        auto *layout = new QVBoxLayout(panel); layout->setContentsMargins(5,10,5,10); layout->setSpacing(3);
        auto *grip = new QLabel("⠿",panel); grip->setAlignment(Qt::AlignCenter); grip->setAttribute(Qt::WA_TransparentForMouseEvents); layout->addWidget(grip);
        marker=button("✎","Маркер (B)",[this]{tool(false);},true); layout->addWidget(marker);
        eraser=button("▱","Губка (E)",[this]{tool(true);},true); layout->addWidget(eraser);
        layout->addWidget(button("●","Цвет маркера",[this]{auto c=QColorDialog::getColor(ink,this,"Цвет маркера");if(c.isValid()) { ink=c; refreshCursors(); }}));
        sizeLabel=new QLabel("5 px",panel); sizeLabel->setAlignment(Qt::AlignCenter); layout->addWidget(sizeLabel);
        sizeSlider=new QSlider(Qt::Horizontal,panel); sizeSlider->setRange(1,50); sizeSlider->setValue(5); sizeSlider->setFixedHeight(28);
        sizeSlider->setStyleSheet("QSlider::groove:horizontal {height:4px;background:#64717b;border-radius:2px;} QSlider::handle:horizontal {background:#f2cc69;width:12px;margin:-6px 0;border-radius:6px;}");
        layout->addWidget(sizeSlider);
        connect(sizeSlider,&QSlider::valueChanged,this,[this](int value){(erasing ? eraserSize : thickness)=value;sizeLabel->setText(QString::number(value)+" px");refreshCursors();});
        layout->addWidget(button("↶","Отменить (Ctrl+Z)",[this]{undo();}));
        layout->addWidget(button("▧","Снимок: в буфер обмена и PNG (Ctrl+S)",[this]{screenshot();}));
        passButton=button("⇢","Пропуск кликов / возврат: Ctrl+Alt+G",[this]{pass();},true); layout->addWidget(passButton);
        layout->addWidget(button("⌫","Стереть всё",[this]{if(!strokes.isEmpty() && QMessageBox::question(this,"Очистить стекло?","Удалить все заметки?")==QMessageBox::Yes){strokes.clear();redoStack.clear();update();}}));
        layout->addWidget(button("×","Выход (Alt+F4)",[this]{close();}));
        status=new QLabel("СТЕКЛО",panel);status->setAlignment(Qt::AlignCenter);layout->addWidget(status);
        auto shortcut=[this](QKeySequence seq,std::function<void()> fn){auto *s=new QShortcut(seq,this);connect(s,&QShortcut::activated,this,fn);};
        shortcut(QKeySequence("B"),[this]{tool(false);}); shortcut(QKeySequence("E"),[this]{tool(true);});
        shortcut(QKeySequence::Undo,[this]{undo();}); shortcut(QKeySequence("Ctrl+Shift+Z"),[this]{redo();}); shortcut(QKeySequence::Save,[this]{screenshot();});
        QSettings s; if(s.contains("geometry")) restoreGeometry(s.value("geometry").toByteArray());
        ink=s.value("color",ink).value<QColor>(); thickness=qBound(1,s.value("width",5).toInt(),50); eraserSize=qBound(1,s.value("eraserSize",35).toInt(),50);
        tool(false); hotkey=RegisterHotKey(reinterpret_cast<HWND>(winId()),1,MOD_CONTROL|MOD_ALT|MOD_NOREPEAT,'G');
    }
    ~Board() override { if(hotkey) UnregisterHotKey(reinterpret_cast<HWND>(winId()),1); }
    bool smoke(const QString &path) {
        tool(false); sizeSlider->setValue(1); bool sizesOk=toolSize()==1;
        sizeSlider->setValue(50); sizesOk &= toolSize()==50;
        tool(true); sizeSlider->setValue(1); sizesOk &= toolSize()==1;
        sizeSlider->setValue(50); sizesOk &= toolSize()==50;
        tool(false); sizesOk &= toolSize()==50;
        sizeSlider->setValue(5); tool(true); sizesOk &= toolSize()==50;
        sizeSlider->setValue(35); tool(false);
        strokes={{{QPointF(40,80),QPointF(180,80)},Qt::red,8,false}};
        auto img=inkImage(); bool ok=sizesOk && qAlpha(img.pixel(90*devicePixelRatioF(),80*devicePixelRatioF()))>0;
        strokes.push_back({{QPointF(90,60),QPointF(90,100)},Qt::black,20,true});
        img=inkImage(); ok &= qAlpha(img.pixel(90*devicePixelRatioF(),80*devicePixelRatioF()))==0;
        undo(); ok &= strokes.size()==1; redo(); ok &= strokes.size()==2;
        resize(720,600); ok &= size()==QSize(720,600); ok &= grab().save(path); strokes.clear(); update(); return ok;
    }
    void smokeCapture(const QString &path, std::function<void(bool)> done) {
        auto *previous=new QMimeData;
        const auto *data=QApplication::clipboard()->mimeData();
        if(data) for(const auto &format:data->formats()) previous->setData(format,data->data(format));
        screenshot([previous,path,done](const QImage &shot){
            const QImage copied=QApplication::clipboard()->image();
            bool ok=!copied.isNull() && copied.size()==shot.size()
                && copied.convertToFormat(QImage::Format_RGB32)==shot.convertToFormat(QImage::Format_RGB32);
            ok &= shot.save(path);
            ok &= QImage(path).convertToFormat(QImage::Format_RGB32)==copied.convertToFormat(QImage::Format_RGB32);
            QApplication::clipboard()->setMimeData(previous);
            done(ok);
        });
    }
};
int main(int argc,char **argv) {
    QApplication app(argc,argv); app.setOrganizationName("Faros");app.setApplicationName("GlassBoard"); app.setStyle("Fusion");
    app.setApplicationVersion(QStringLiteral(GLASSBOARD_VERSION));
    if (app.arguments().contains("--version")) { QTextStream(stdout) << app.applicationVersion() << Qt::endl; return 0; }
    const int dataIndex = app.arguments().indexOf("--data-dir");
    if (dataIndex >= 0) {
        if (dataIndex + 1 >= app.arguments().size() || app.arguments().at(dataIndex + 1).startsWith("--")) return 3;
        const QString root = QDir(app.arguments().at(dataIndex + 1)).absolutePath();
        if (!QDir().mkpath(root)) return 3;
        QSettings::setDefaultFormat(QSettings::IniFormat);
        QSettings::setPath(QSettings::IniFormat,QSettings::UserScope,root);
    }
    QString smokeOutput;
    if (app.arguments().contains("--smoke-test")) {
        if (dataIndex < 0) return 3;
        const int outputIndex = app.arguments().indexOf("--smoke-output");
        if (outputIndex < 0 || outputIndex + 1 >= app.arguments().size()) return 3;
        smokeOutput = QDir(app.arguments().at(outputIndex + 1)).absolutePath();
        if (!QDir().mkpath(smokeOutput)) return 3;
    }
    Board board; board.show();
    if(app.arguments().contains("--smoke-test")) QTimer::singleShot(350,&board,[&]{bool ok=board.smoke(smokeOutput+"/smoke.png"); board.smokeCapture(smokeOutput+"/capture.png",[&,ok](bool captureOk) mutable { ok &= captureOk; QFile f(smokeOutput+"/smoke-result.txt"); ok &= f.open(QIODevice::WriteOnly); if(f.isOpen()) f.write(ok?"PASS: ink, eraser, undo, redo, resize, sizes 1-50, screenshot clipboard and PNG\n":"FAIL\n"); QJsonObject result{{"version",app.applicationVersion()},{"windowTitle",board.windowTitle()},{"passed",ok}}; QFile report(smokeOutput+"/report.json"); if(report.open(QIODevice::WriteOnly)) report.write(QJsonDocument(result).toJson()); else ok=false; app.exit(ok?0:1);});});
    return app.exec();
}
