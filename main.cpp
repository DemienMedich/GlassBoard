#include <QtWidgets>
#include <windows.h>
#include "board_controls.h"
#include "board_document.h"
#include <optional>

class Board : public QWidget {
    QVector<Stroke> strokes, redoStack;
    QWidget *panel;
    QLabel *status;
    QPushButton *marker, *eraser, *passButton;
    QPushButton *arrowButton, *rectangleButton, *textButton, *glassButton;
    QPushButton *undoButton, *redoButton;
    DrawingTool activeTool=DrawingTool::Marker;
    std::optional<Stroke> draft;
    QPlainTextEdit *textEditor=nullptr;
    QColor textInk;
    int textPixels=24;
    bool solidGlass=false, spaceHeld=false;
    QVector<QShortcut *> toolShortcuts;
    const QColor boardColor=QColor("#202830");
    void updateHistoryButtons() { undoButton->setEnabled(!strokes.isEmpty());redoButton->setEnabled(!redoStack.isEmpty()); }
    void commitText(bool accept=true) {
        if(!textEditor)return;
        auto *editor=textEditor;textEditor=nullptr;
        const QString text=editor->toPlainText();
        if(accept && !text.trimmed().isEmpty()) {
            const QPointF position=editor->pos()+QPoint(8,8);
            strokes.push_back({{position,position+QPointF(editor->viewport()->width()-16,0)},textInk,qreal(textPixels),false,StrokeKind::Text,text});
            redoStack.clear();updateHistoryButtons();
        }
        editor->hide();editor->deleteLater();for(auto *shortcut:toolShortcuts)shortcut->setEnabled(true);update();
    }
    void beginText(QPoint position) {
        commitText();
        const QRect bounds=canvas();
        position.setX(qBound(bounds.left(),position.x(),qMax(bounds.left(),bounds.right()-100)));
        position.setY(qBound(bounds.top(),position.y(),qMax(bounds.top(),bounds.bottom()-80)));
        textEditor=new QPlainTextEdit(this);textInk=ink;textPixels=textSize;
        textEditor->setGeometry(QRect(position,QSize(qMin(360,bounds.right()-position.x()+1),qMin(160,bounds.bottom()-position.y()+1))));
        textEditor->setStyleSheet("QPlainTextEdit {background:#202830;color:"+ink.name()+";border:1px solid #f2cc69;border-radius:6px;padding:6px;}");
        QFont font("Segoe UI");font.setPixelSize(textSize);textEditor->setFont(font);textEditor->document()->setDocumentMargin(0);
        textEditor->setPlaceholderText("Введите текст…");textEditor->setAccessibleName("Текст заметки");
        textEditor->installEventFilter(this);for(auto *shortcut:toolShortcuts)shortcut->setEnabled(false);
        textEditor->show();textEditor->setFocus();
        notify("Enter — готово · Shift+Enter — новая строка · Esc — отмена");
    }
    void chooseTool(DrawingTool value) {
        commitText();draft.reset();drawing=false;activeTool=value;erasing=value==DrawingTool::Eraser;
        marker->setChecked(value==DrawingTool::Marker);eraser->setChecked(erasing);
        arrowButton->setChecked(value==DrawingTool::Arrow);rectangleButton->setChecked(value==DrawingTool::Rectangle);textButton->setChecked(value==DrawingTool::Text);
        syncSizeUi();refreshCursors();update();
    }
    void toggleGlass() {
        commitText();solidGlass=!solidGlass;glassButton->setChecked(solidGlass);
        status->setText(passing?"СКВОЗЬ":solidGlass?"ДОСКА":"СТЕКЛО");
        notify(solidGlass?"Непрозрачная доска · G — вернуть стекло":"Прозрачное стекло · G — непрозрачная доска");update();
    }
    void updateDraft(QPointF point, Qt::KeyboardModifiers modifiers) {
        if(!draft)return;
        if(draft->kind==StrokeKind::Arrow || draft->kind==StrokeKind::Rectangle) {
            if(modifiers.testFlag(Qt::ShiftModifier))point=constrainedEnd(draft->points[0],point,draft->kind==StrokeKind::Rectangle);
            draft->points[1]=point;
        }else if(draft->points.last()!=point)draft->points<<point;
        update();
    }
    void finishDraft() {
        if(draft && (draft->kind==StrokeKind::Freehand || QLineF(draft->points[0],draft->points[1]).length()>.5)){
            strokes.push_back(*draft);redoStack.clear();
        }
        draft.reset();drawing=false;updateHistoryButtons();update();
    }
    void clearBoard() {
        commitText();if(strokes.isEmpty())return;
        if(QMessageBox::question(this,"Очистить стекло?","Удалить все заметки?")==QMessageBox::Yes){strokes.clear();redoStack.clear();draft.reset();updateHistoryButtons();notify("Стекло очищено");update();}
    }
    void showHelp() {
        commitText();QMessageBox::information(this,"Быстрые команды GlassBoard",
            "B — маркер    E — губка\nA — стрелка    R — прямоугольник    T — текст\n"
            "G — стекло / непрозрачная доска    C — палитра\n"
            "Q — настройки инструмента у курсора\n"
            "Shift при рисовании — квадрат / стрелка с шагом 45°\n"
            "[ / ] или Ctrl+колесо — размер инструмента\n"
            "Пробел + перетаскивание / средняя кнопка — переместить окно\n"
            "Enter — закончить текст    Shift+Enter — новая строка\nEsc — отменить ввод / незавершённую фигуру\n"
            "Ctrl+Z — отменить    Ctrl+Shift+Z / Ctrl+Y — повторить\n"
            "Ctrl+S — сохранить PNG    Ctrl+Shift+C — копировать снимок\n"
            "Ctrl+Shift+Delete — очистить с подтверждением\nCtrl+Alt+G — пропуск кликов / вернуть рисование\n\n"
            "В режиме текста буквенные команды отключены до завершения ввода.");
    }
    QColor ink = QColor("#ffcf56");
    int thickness = 5, eraserSize = 35, textSize = 24;
    QSlider *sizeSlider;
    QLabel *sizeLabel;
    QSpinBox *sizeNumber;
    QFrame *sizePopup, *palettePopup;
    QFrame *quickPopup=nullptr;
    QSlider *quickSlider=nullptr;
    QSpinBox *quickNumber=nullptr;
    QLabel *quickTitle=nullptr;
    BrushPreview *quickPreview=nullptr;
    QWidget *quickColors=nullptr;
    BrushPreview *preview;
    IconButton *colorButton, *sizeButton;
    DragHandle *handle;
    QLabel *toast;
    QTimer toastTimer;
    void notify(const QString &text, bool persistent = false) {
        toastTimer.stop(); toast->setFixedWidth(qBound(200,width()-40,520));toast->setWordWrap(true);toast->setText(text); toast->adjustSize();
        toast->move(qMax(10,(width()-toast->width())/2),height()-toast->height()-20);
        toast->show(); toast->raise(); if(!persistent) toastTimer.start(2600);
    }
    void syncSizeUi() {
        QSignalBlocker sliderBlock(sizeSlider), numberBlock(sizeNumber);
        const bool text=activeTool==DrawingTool::Text;
        sizeSlider->setRange(text?8:1,text?72:50);sizeNumber->setRange(text?8:1,text?72:50);
        sizeSlider->setValue(toolSize()); sizeNumber->setValue(toolSize());
        sizeLabel->setText(erasing?"Размер губки":text?"Размер текста":"Толщина линии");
        preview->diameter=toolSize();preview->erase=erasing;preview->text=text;preview->ink=ink;preview->update();
        sizeButton->setToolTip(QString("%1: %2 px · [ / ] или Ctrl+колесо").arg(erasing?"Губка":text?"Текст":"Линия").arg(toolSize()));
        if(quickSlider){
            QSignalBlocker sliderBlock(quickSlider),numberBlock(quickNumber);
            quickSlider->setRange(text?8:1,text?72:50);quickNumber->setRange(text?8:1,text?72:50);
            quickSlider->setValue(toolSize());quickNumber->setValue(toolSize());
            const QString name=erasing?"Губка":text?"Текст":activeTool==DrawingTool::Arrow?"Стрелка":activeTool==DrawingTool::Rectangle?"Прямоугольник":"Маркер";
            quickTitle->setText(name+" · настройки");quickColors->setVisible(!erasing);
            quickPopup->setFixedHeight(erasing?220:360);
            quickPreview->diameter=toolSize();quickPreview->erase=erasing;quickPreview->text=text;quickPreview->ink=ink;quickPreview->update();
        }
    }
    void setInk(QColor color) { if(color.isValid()){ink=color;colorButton->accent=color;colorButton->update();syncSizeUi();refreshCursors();} }
    void openPopup(QFrame *popup) {
        if(popup->isVisible()){popup->hide();return;}
        sizePopup->hide();palettePopup->hide();if(quickPopup)quickPopup->hide();
        QPoint pos=panel->mapToGlobal(QPoint(-popup->width()-10,30));
        const QRect available=screen()->availableGeometry();
        pos.setX(qBound(available.left(),pos.x(),qMax(available.left(),available.right()-popup->width()+1)));
        pos.setY(qBound(available.top(),pos.y(),qMax(available.top(),available.bottom()-popup->height()+1)));
        popup->move(pos);popup->show();popup->raise();
    }
    void quickSettings(QPoint anchor=QCursor::pos()) {
        if(textEditor)return;
        if(quickPopup->isVisible()){quickPopup->hide();setFocus();return;}
        if(draft)finishDraft();syncSizeUi();sizePopup->hide();palettePopup->hide();
        const QRect bounds=QGuiApplication::screenAt(anchor)?QGuiApplication::screenAt(anchor)->availableGeometry():screen()->availableGeometry();
        QPoint position=anchor+QPoint(12,12);
        if(position.x()+quickPopup->width()>bounds.right()+1)position.setX(anchor.x()-quickPopup->width()-12);
        if(position.y()+quickPopup->height()>bounds.bottom()+1)position.setY(anchor.y()-quickPopup->height()-12);
        position.setX(qBound(bounds.left(),position.x(),qMax(bounds.left(),bounds.right()-quickPopup->width()+1)));
        position.setY(qBound(bounds.top(),position.y(),qMax(bounds.top(),bounds.bottom()-quickPopup->height()+1)));
        quickPopup->move(position);quickPopup->show();quickPopup->raise();quickSlider->setFocus();
    }
    int toolSize() const { return erasing ? eraserSize : activeTool==DrawingTool::Text?textSize:thickness; }
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
        else if(canvas().contains(pt)) {
            if(spaceHeld)setCursor(Qt::OpenHandCursor);
            else if(activeTool==DrawingTool::Text)setCursor(Qt::IBeamCursor);
            else if(activeTool==DrawingTool::Arrow || activeTool==DrawingTool::Rectangle)setCursor(Qt::CrossCursor);
            else setCursor(erasing?spongeCursor:markerCursor);
        }
        else setCursor(Qt::SizeAllCursor);
    }
    QRect canvas() const { return rect().adjusted(8, 50, -132, -8); }
    void renderInk(QPainter &p) const {
        for (const auto &s : strokes) paintStroke(p,s);
        if(draft)paintStroke(p,*draft);
    }
    QImage inkImage() const {
        QImage img(size() * devicePixelRatioF(), QImage::Format_ARGB32_Premultiplied);
        img.setDevicePixelRatio(devicePixelRatioF()); img.fill(Qt::transparent);
        QPainter p(&img); p.setRenderHint(QPainter::Antialiasing); p.setClipRect(canvas()); renderInk(p); return img;
    }
    IconButton *button(ToolIcon icon, QString tip, std::function<void()> action, bool check = false) {
        auto *b = new IconButton(icon, panel);
        b->setToolTip(tip);b->setAccessibleName(tip); b->setCheckable(check); connect(b, &QPushButton::clicked, this, [this,action]{setFocus();action();}); return b;
    }
    void tool(bool erase) {
        chooseTool(erase?DrawingTool::Eraser:DrawingTool::Marker);
    }
    void undo() { commitText();if(draft){draft.reset();drawing=false;}else if (!strokes.isEmpty())redoStack.push_back(strokes.takeLast());updateHistoryButtons();update(); }
    void redo() { commitText();if (!redoStack.isEmpty())strokes.push_back(redoStack.takeLast());updateHistoryButtons();update(); }
    void pass() {
        commitText();draft.reset();drawing=false;
        if (!hotkey) { QMessageBox::information(this, "GlassBoard", "Ctrl+Alt+G занята другой программой. Режим пропуска кликов недоступен."); return; }
        passing = !passing;
        auto hwnd = reinterpret_cast<HWND>(winId());
        auto ex = GetWindowLongPtr(hwnd, GWL_EXSTYLE);
        SetWindowLongPtr(hwnd, GWL_EXSTYLE, passing ? ex | WS_EX_TRANSPARENT : ex & ~WS_EX_TRANSPARENT);
        passButton->setChecked(passing); status->setText(passing ? "СКВОЗЬ" : solidGlass?"ДОСКА":"СТЕКЛО"); update();
        notify(passing?"Клики проходят сквозь окно · Ctrl+Alt+G — вернуть рисование":"Рисование включено",passing);
    }
    void screenshot(bool toClipboard, std::function<void(const QImage &)> completed = {}, QString savePath = {}) {
        commitText();if(draft)finishDraft();
        const QRect area(mapToGlobal(canvas().topLeft()), canvas().size());
        const QImage overlay = inkImage().copy(QRect(canvas().topLeft() * devicePixelRatioF(), canvas().size() * devicePixelRatioF()));
        const bool solid=solidGlass;
        sizePopup->hide();palettePopup->hide();quickPopup->hide();
        hide();
        QTimer::singleShot(220, this, [this, area, overlay, solid, toClipboard, completed, savePath] {
            QImage shot(area.size() * devicePixelRatioF(), QImage::Format_ARGB32_Premultiplied);
            shot.setDevicePixelRatio(devicePixelRatioF()); shot.fill(solid?boardColor:Qt::black);
            { QPainter p(&shot);
              if(!solid) for (auto *screen : QGuiApplication::screens()) {
                  const QRect part = area.intersected(screen->geometry());
                  if (part.isEmpty()) continue;
                  const QPoint pos = part.topLeft() - screen->geometry().topLeft();
                  const QPixmap pixels = screen->grabWindow(0, pos.x(), pos.y(), part.width(), part.height());
                  p.drawPixmap(QRect(part.topLeft() - area.topLeft(), part.size()), pixels);
              }
              p.drawImage(QPoint(0, 0), overlay);
            }
            show();
            if(toClipboard) {
                QApplication::clipboard()->setImage(shot);
                notify("Снимок скопирован · Ctrl+V — вставить");
            } else {
                const auto path = !savePath.isEmpty() ? savePath : QFileDialog::getSaveFileName(this, "Сохранить снимок PNG", QStandardPaths::writableLocation(QStandardPaths::PicturesLocation) + "/GlassBoard_" + QDateTime::currentDateTime().toString("yyyy-MM-dd_HH-mm-ss") + ".png", "PNG (*.png)");
                if (!path.isEmpty()) {
                    if(!shot.save(path)) QMessageBox::warning(this, "GlassBoard", "Не удалось сохранить изображение.");
                    else notify("Снимок сохранён в PNG");
                }
            }
            if(completed) completed(shot);
        });
    }
protected:
    void contextMenuEvent(QContextMenuEvent *event) override {
        if(canvas().contains(event->pos()) && !textEditor){quickSettings(event->globalPos());event->accept();return;}
        QWidget::contextMenuEvent(event);
    }
    bool eventFilter(QObject *object,QEvent *event) override {
        if(quickPopup && quickPopup->isVisible() && event->type()==QEvent::KeyPress){
            auto *widget=qobject_cast<QWidget *>(object);auto *key=static_cast<QKeyEvent *>(event);
            if(widget && (widget==quickPopup || quickPopup->isAncestorOf(widget)) && (key->key()==Qt::Key_Escape || (key->key()==Qt::Key_Q && key->modifiers()==Qt::NoModifier))){if(!key->isAutoRepeat()){quickPopup->hide();setFocus();}return true;}
        }
        if(object==textEditor && event->type()==QEvent::KeyPress){
            auto *key=static_cast<QKeyEvent *>(event);
            if(key->key()==Qt::Key_Escape){commitText(false);setFocus();return true;}
            if((key->key()==Qt::Key_Return||key->key()==Qt::Key_Enter) && !key->modifiers().testFlag(Qt::ShiftModifier)){commitText();setFocus();return true;}
        }
        if(object==textEditor && event->type()==QEvent::FocusOut){
            QPointer<QPlainTextEdit> editor=textEditor;
            QTimer::singleShot(0,this,[this,editor]{if(editor && editor==textEditor && !editor->hasFocus())commitText();});
        }
        return QWidget::eventFilter(object,event);
    }
    void keyPressEvent(QKeyEvent *event) override {
        if(event->key()==Qt::Key_Space && !event->isAutoRepeat()){spaceHeld=true;updateCursorAt(mapFromGlobal(QCursor::pos()));event->accept();return;}
        if(event->key()==Qt::Key_Escape){draft.reset();drawing=false;update();event->accept();return;}
        QWidget::keyPressEvent(event);
    }
    void keyReleaseEvent(QKeyEvent *event) override {
        if(event->key()==Qt::Key_Space && !event->isAutoRepeat()){spaceHeld=false;updateCursorAt(mapFromGlobal(QCursor::pos()));event->accept();return;}
        QWidget::keyReleaseEvent(event);
    }
    void focusOutEvent(QFocusEvent *event) override {spaceHeld=false;QWidget::focusOutEvent(event);}
    void wheelEvent(QWheelEvent *event) override {
        if(event->modifiers().testFlag(Qt::ControlModifier)){sizeSlider->setValue(toolSize()+(event->angleDelta().y()>0?1:-1));event->accept();return;}
        QWidget::wheelEvent(event);
    }
    bool nativeEvent(const QByteArray &, void *message, qintptr *result) override {
        auto *msg = static_cast<MSG *>(message);
        if (msg->message == WM_HOTKEY && msg->wParam == 1) { pass(); *result = 0; return true; }
        return false;
    }
    void resizeEvent(QResizeEvent *) override {
        panel->move(width() - panel->width()-10, (height() - panel->height()) / 2);
        handle->move((width()-handle->width())/2,10);
        if(toast->isVisible())toast->move(qMax(10,(width()-toast->width())/2),height()-toast->height()-20);
        if(sizePopup)sizePopup->hide();if(palettePopup)palettePopup->hide();if(quickPopup)quickPopup->hide();update();
    }
    void paintEvent(QPaintEvent *) override {
        QPainter p(this); p.setRenderHint(QPainter::Antialiasing);
        p.setPen(Qt::NoPen); p.setBrush(QColor(210, 233, 242, passing ? 3 : 12)); p.drawRoundedRect(rect().adjusted(6,6,-6,-6), 12,12);
        if(solidGlass){p.setBrush(boardColor);p.drawRect(canvas());}
        QLinearGradient frame(0,0,width(),height()); frame.setColorAt(0, QColor(228,235,239,225)); frame.setColorAt(.5, QColor(97,111,122,220)); frame.setColorAt(1,QColor(199,211,220,235));
        p.setBrush(Qt::NoBrush); p.setPen(QPen(frame, 4)); p.drawRoundedRect(rect().adjusted(4,4,-4,-4), 14,14);
        p.setPen(QPen(QColor(255,255,255,150),1)); p.drawRoundedRect(rect().adjusted(7,7,-7,-7),10,10);
        p.drawImage(0,0,inkImage());
        p.setPen(QPen(QColor(228,236,242,180),2));
        for(int i=0;i<3;++i) p.drawLine(width()-14-i*4,height()-10,width()-10,height()-14-i*4);
    }
    void mousePressEvent(QMouseEvent *e) override {
        if(e->button()==Qt::MiddleButton){commitText();windowHandle()->startSystemMove();return;}
        if (e->button() != Qt::LeftButton) return;
        const QPoint pt = e->position().toPoint(); Qt::Edges edges;
        if (pt.x()<8) edges |= Qt::LeftEdge; if(pt.x()>width()-9) edges |= Qt::RightEdge;
        if (pt.y()<8) edges |= Qt::TopEdge; if(pt.y()>height()-9) edges |= Qt::BottomEdge;
        if(edges) { windowHandle()->startSystemResize(edges); return; }
        if (!canvas().contains(pt) || spaceHeld || e->modifiers().testFlag(Qt::AltModifier)) { commitText();windowHandle()->startSystemMove(); return; }
        if(activeTool==DrawingTool::Text){beginText(pt);return;}
        commitText();setFocus();
        StrokeKind kind=activeTool==DrawingTool::Arrow?StrokeKind::Arrow:activeTool==DrawingTool::Rectangle?StrokeKind::Rectangle:StrokeKind::Freehand;
        draft=Stroke{QPolygonF{e->position()},ink,qreal(toolSize()),erasing,kind};
        if(kind!=StrokeKind::Freehand)draft->points<<e->position();drawing=true;update();
    }
    void mouseMoveEvent(QMouseEvent *e) override {
        if(drawing) { updateDraft(e->position(),e->modifiers()); return; }
        updateCursorAt(e->position().toPoint());
    }
    void mouseReleaseEvent(QMouseEvent *e) override {
        if(e->button()!=Qt::LeftButton || !drawing)return;
        updateDraft(e->position(),e->modifiers());finishDraft();
    }
    void closeEvent(QCloseEvent *) override {commitText(); QSettings s; s.setValue("geometry", saveGeometry()); s.setValue("color",ink); s.setValue("width",thickness); s.setValue("eraserSize",eraserSize);s.setValue("textSize",textSize);s.setValue("solidGlass",solidGlass); }
public:
    Board() {
        setWindowTitle("GlassBoard " + QCoreApplication::applicationVersion()); setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Window);
        setAttribute(Qt::WA_TranslucentBackground); setMouseTracking(true);setFocusPolicy(Qt::StrongFocus); setMinimumSize(360,560); resize(1000,680);
        panel = new QWidget(this); panel->setObjectName("tools"); panel->setFixedSize(112,514);
        panel->setStyleSheet("QWidget#tools { background:#202830; border:1px solid #64717b; border-radius:16px; } QLabel {color:#acbac4; font:8px 'Segoe UI';}");
        handle=new DragHandle(this);
        toast=new QLabel(this);toast->setAttribute(Qt::WA_TransparentForMouseEvents);
        toast->setStyleSheet("background:#202830;color:#e4edf3;border:1px solid #64717b;border-radius:10px;padding:10px 14px;font:12px 'Segoe UI';");toast->hide();
        toastTimer.setSingleShot(true);connect(&toastTimer,&QTimer::timeout,toast,&QWidget::hide);
        const QString popupStyle="QFrame#popup {background:#202830;border:1px solid #64717b;border-radius:12px;} QLabel {color:#e4edf3;font:12px 'Segoe UI';} QPushButton {color:#e4edf3;background:#36444f;border:0;border-radius:8px;padding:8px;} QPushButton:hover {background:#485b68;} QSpinBox {color:#e4edf3;background:#141b21;border:1px solid #64717b;border-radius:6px;padding:6px;font:13px 'Segoe UI';}";
        sizePopup=new QFrame(this,Qt::Popup|Qt::FramelessWindowHint);sizePopup->setObjectName("popup");sizePopup->setStyleSheet(popupStyle);sizePopup->setFixedSize(260,196);
        auto *sizeLayout=new QVBoxLayout(sizePopup);sizeLayout->setContentsMargins(16,14,16,14);
        auto *sizeRow=new QHBoxLayout;sizeLabel=new QLabel(sizePopup);sizeRow->addWidget(sizeLabel);sizeRow->addStretch();
        sizeNumber=new QSpinBox(sizePopup);sizeNumber->setRange(1,50);sizeNumber->setSuffix(" px");sizeNumber->setAccessibleName("Точный размер инструмента");sizeRow->addWidget(sizeNumber);sizeLayout->addLayout(sizeRow);
        sizeSlider=new QSlider(Qt::Horizontal,sizePopup);sizeSlider->setRange(1,50);sizeSlider->setValue(5);sizeSlider->setMinimumHeight(32);sizeSlider->setAccessibleName("Размер инструмента от 1 до 50");
        sizeSlider->setStyleSheet("QSlider::groove:horizontal {height:4px;background:#64717b;border-radius:2px;} QSlider::handle:horizontal {background:#f2cc69;width:16px;margin:-7px 0;border-radius:8px;}");sizeLayout->addWidget(sizeSlider);
        preview=new BrushPreview(sizePopup);sizeLayout->addWidget(preview);
        connect(sizeSlider,&QSlider::valueChanged,this,[this](int value){(erasing?eraserSize:activeTool==DrawingTool::Text?textSize:thickness)=value;syncSizeUi();refreshCursors();});
        connect(sizeNumber,qOverload<int>(&QSpinBox::valueChanged),sizeSlider,&QSlider::setValue);
        palettePopup=new QFrame(this,Qt::Popup|Qt::FramelessWindowHint);palettePopup->setObjectName("popup");palettePopup->setStyleSheet(popupStyle);palettePopup->setFixedSize(224,188);
        auto *paletteLayout=new QVBoxLayout(palettePopup);paletteLayout->setContentsMargins(14,12,14,12);paletteLayout->addWidget(new QLabel("Цвет заметок",palettePopup));
        auto *swatches=new QGridLayout;swatches->setSpacing(4);
        const QStringList colors={"#ffcf56","#f26e75","#70b5ff","#79d5aa","#ffffff","#202830","#bd98f7","#f3a66b"};
        for(int i=0;i<colors.size();++i){auto *swatch=new IconButton(ToolIcon::Color,palettePopup);swatch->accent=QColor(colors[i]);swatch->setToolTip(colors[i]);swatch->setAccessibleName("Цвет "+colors[i]);swatches->addWidget(swatch,i/4,i%4);connect(swatch,&QPushButton::clicked,this,[this,color=colors[i]]{setInk(QColor(color));palettePopup->hide();});}
        paletteLayout->addLayout(swatches);auto *customColor=new QPushButton("Другой цвет…",palettePopup);paletteLayout->addWidget(customColor);
        connect(customColor,&QPushButton::clicked,this,[this]{palettePopup->hide();setInk(QColorDialog::getColor(ink,this,"Цвет заметок"));});
        quickPopup=new QFrame(this,Qt::Popup|Qt::FramelessWindowHint);quickPopup->setObjectName("popup");quickPopup->setCursor(Qt::ArrowCursor);quickPopup->setStyleSheet(popupStyle);quickPopup->setFixedSize(280,360);
        auto *quickLayout=new QVBoxLayout(quickPopup);quickLayout->setContentsMargins(14,12,14,12);quickLayout->setSpacing(6);
        quickTitle=new QLabel("Маркер · настройки",quickPopup);quickLayout->addWidget(quickTitle);
        auto *quickRow=new QHBoxLayout;quickRow->addWidget(new QLabel("Размер",quickPopup));quickRow->addStretch();
        quickNumber=new QSpinBox(quickPopup);quickNumber->setSuffix(" px");quickNumber->setAccessibleName("Быстрый размер инструмента");quickRow->addWidget(quickNumber);quickLayout->addLayout(quickRow);
        quickSlider=new QSlider(Qt::Horizontal,quickPopup);quickSlider->setMinimumHeight(32);quickSlider->setAccessibleName("Быстрая настройка размера");quickSlider->setStyleSheet(sizeSlider->styleSheet());quickLayout->addWidget(quickSlider);
        quickPreview=new BrushPreview(quickPopup);quickPreview->setFixedHeight(44);quickLayout->addWidget(quickPreview);
        quickColors=new QWidget(quickPopup);auto *colorsLayout=new QVBoxLayout(quickColors);colorsLayout->setContentsMargins(0,0,0,0);colorsLayout->setSpacing(6);
        auto *quickSwatches=new QGridLayout;quickSwatches->setSpacing(4);
        for(int i=0;i<colors.size();++i){auto *swatch=new IconButton(ToolIcon::Color,quickColors);swatch->accent=QColor(colors[i]);swatch->setToolTip(colors[i]);swatch->setAccessibleName("Быстрый цвет "+colors[i]);quickSwatches->addWidget(swatch,i/4,i%4);connect(swatch,&QPushButton::clicked,this,[this,color=colors[i]]{setInk(QColor(color));});}
        colorsLayout->addLayout(quickSwatches);auto *quickCustom=new QPushButton("Другой цвет…",quickColors);colorsLayout->addWidget(quickCustom);
        connect(quickCustom,&QPushButton::clicked,this,[this]{quickPopup->hide();setInk(QColorDialog::getColor(ink,this,"Цвет заметок"));});quickLayout->addWidget(quickColors);
        auto *quickHint=new QLabel("Q / Esc — закрыть · настройки применяются сразу",quickPopup);quickHint->setWordWrap(true);quickHint->setStyleSheet("color:#acbac4;font:11px 'Segoe UI';");quickLayout->addWidget(quickHint);
        connect(quickSlider,&QSlider::valueChanged,sizeSlider,&QSlider::setValue);connect(quickNumber,qOverload<int>(&QSpinBox::valueChanged),sizeSlider,&QSlider::setValue);
        quickPopup->installEventFilter(this);for(auto *widget:quickPopup->findChildren<QWidget *>())widget->installEventFilter(this);
        auto *layout = new QVBoxLayout(panel); layout->setContentsMargins(8,10,8,10); layout->setSpacing(2);
        auto section=[&](QString text){auto *label=new QLabel(text,panel);label->setAlignment(Qt::AlignCenter);label->setFixedHeight(20);layout->addWidget(label);};
        auto divider=[&]{auto *line=new QFrame(panel);line->setFixedHeight(1);line->setStyleSheet("background:#44515b;");layout->addSpacing(5);layout->addWidget(line);layout->addSpacing(5);};
        section("ИНСТРУМЕНТЫ");
        auto *tools=new QGridLayout;tools->setSpacing(4);layout->addLayout(tools);
        marker=button(ToolIcon::Marker,"Маркер (B)",[this]{tool(false);},true);tools->addWidget(marker,0,0);
        eraser=button(ToolIcon::Sponge,"Губка (E)",[this]{tool(true);},true);tools->addWidget(eraser,0,1);
        arrowButton=button(ToolIcon::Arrow,"Стрелка (A) · Shift — шаг 45°",[this]{chooseTool(DrawingTool::Arrow);},true);tools->addWidget(arrowButton,1,0);
        rectangleButton=button(ToolIcon::Rectangle,"Прямоугольник (R) · Shift — квадрат",[this]{chooseTool(DrawingTool::Rectangle);},true);tools->addWidget(rectangleButton,1,1);
        textButton=button(ToolIcon::Text,"Текст (T) · клик на доске для ввода",[this]{chooseTool(DrawingTool::Text);},true);tools->addWidget(textButton,2,0);
        glassButton=button(ToolIcon::Glass,"Непрозрачная доска / стекло (G)",[this]{toggleGlass();},true);tools->addWidget(glassButton,2,1);
        colorButton=button(ToolIcon::Color,"Цвет заметок (C)",[this]{openPopup(palettePopup);});tools->addWidget(colorButton,3,0);
        sizeButton=button(ToolIcon::Size,"Размер инструмента",[this]{syncSizeUi();openPopup(sizePopup);});tools->addWidget(sizeButton,3,1);
        divider();section("ДЕЙСТВИЯ");
        auto *actions=new QGridLayout;actions->setSpacing(4);layout->addLayout(actions);
        undoButton=button(ToolIcon::Undo,"Отменить (Ctrl+Z)",[this]{undo();});actions->addWidget(undoButton,0,0);
        redoButton=button(ToolIcon::Redo,"Повторить (Ctrl+Shift+Z / Ctrl+Y)",[this]{redo();});actions->addWidget(redoButton,0,1);
        actions->addWidget(button(ToolIcon::Camera,"Сохранить снимок PNG (Ctrl+S)",[this]{screenshot(false);}),1,0);
        actions->addWidget(button(ToolIcon::Clipboard,"Копировать снимок (Ctrl+Shift+C)",[this]{screenshot(true);}),1,1);
        passButton=button(ToolIcon::Pass,"Пропуск кликов / возврат: Ctrl+Alt+G",[this]{pass();},true);actions->addWidget(passButton,2,0);
        actions->addWidget(button(ToolIcon::Clear,"Стереть всё (Ctrl+Shift+Delete)",[this]{clearBoard();}),2,1);
        divider();auto *bottom=new QHBoxLayout;layout->addLayout(bottom);
        bottom->addWidget(button(ToolIcon::Help,"Быстрые команды (F1)",[this]{showHelp();}));
        bottom->addWidget(button(ToolIcon::Close,"Выход (Alt+F4)",[this]{close();}));
        status=new QLabel("СТЕКЛО",panel);status->setAlignment(Qt::AlignCenter);layout->addWidget(status);
        auto shortcut=[this](QKeySequence seq,std::function<void()> fn,bool editingSafe=false){auto *s=new QShortcut(seq,this);if(seq==QKeySequence("Q"))s->setAutoRepeat(false);connect(s,&QShortcut::activated,this,fn);if(!editingSafe)toolShortcuts<<s;};
        shortcut(QKeySequence("B"),[this]{tool(false);}); shortcut(QKeySequence("E"),[this]{tool(true);});
        shortcut(QKeySequence("A"),[this]{chooseTool(DrawingTool::Arrow);});shortcut(QKeySequence("R"),[this]{chooseTool(DrawingTool::Rectangle);});shortcut(QKeySequence("T"),[this]{chooseTool(DrawingTool::Text);});
        shortcut(QKeySequence("G"),[this]{toggleGlass();});shortcut(QKeySequence("C"),[this]{openPopup(palettePopup);});
        shortcut(QKeySequence("Q"),[this]{quickSettings();});
        shortcut(QKeySequence("["),[this]{sizeSlider->setValue(toolSize()-1);});shortcut(QKeySequence("]"),[this]{sizeSlider->setValue(toolSize()+1);});
        shortcut(QKeySequence("F1"),[this]{showHelp();});
        shortcut(QKeySequence::Undo,[this]{undo();}); shortcut(QKeySequence("Ctrl+Shift+Z"),[this]{redo();});shortcut(QKeySequence("Ctrl+Y"),[this]{redo();});
        shortcut(QKeySequence::Save,[this]{screenshot(false);},true);shortcut(QKeySequence("Ctrl+Shift+C"),[this]{screenshot(true);},true);
        shortcut(QKeySequence("Ctrl+Shift+Delete"),[this]{clearBoard();});
        QSettings s; if(s.contains("geometry")) restoreGeometry(s.value("geometry").toByteArray());
        ink=s.value("color",ink).value<QColor>();colorButton->accent=ink; thickness=qBound(1,s.value("width",5).toInt(),50); eraserSize=qBound(1,s.value("eraserSize",35).toInt(),50);
        textSize=qBound(8,s.value("textSize",24).toInt(),72);solidGlass=s.value("solidGlass",false).toBool();glassButton->setChecked(solidGlass);updateHistoryButtons();
        status->setText(solidGlass?"ДОСКА":"СТЕКЛО");
        tool(false); hotkey=RegisterHotKey(reinterpret_cast<HWND>(winId()),1,MOD_CONTROL|MOD_ALT|MOD_NOREPEAT,'G');
    }
    ~Board() override { if(hotkey) UnregisterHotKey(reinterpret_cast<HWND>(winId()),1); }
    bool smoke(const QString &path) {
        const QString evidence=QFileInfo(path).absolutePath();
        QFile diagnostics(evidence+"/checks.txt");diagnostics.open(QIODevice::WriteOnly);QTextStream checks(&diagnostics);
        auto checkpoint=[&](const char *name,bool state){checks<<name<<": "<<(state?"PASS":"FAIL")<<Qt::endl;};
        QShortcut *quickKey=nullptr;for(auto *key:toolShortcuts)if(key->key()==QKeySequence("Q"))quickKey=key;
        bool quickOk=quickKey!=nullptr;
        if(quickKey)QMetaObject::invokeMethod(quickKey,"activated",Qt::DirectConnection);
        quickOk &= quickPopup->isVisible();
        quickNumber->setValue(13);quickOk &= toolSize()==13 && sizeSlider->value()==13 && quickSlider->value()==13 && quickPreview->diameter==13;
        for(auto *swatch:quickColors->findChildren<QPushButton *>())if(swatch->accessibleName()=="Быстрый цвет #f26e75")swatch->click();
        quickOk &= ink==QColor("#f26e75") && quickPreview->ink==ink;
        quickPopup->grab().save(evidence+"/quick-settings.png");
        QKeyEvent closeQuick(QEvent::KeyPress,Qt::Key_Q,Qt::NoModifier);QApplication::sendEvent(quickNumber,&closeQuick);quickOk &= !quickPopup->isVisible();
        chooseTool(DrawingTool::Eraser);quickSettings(screen()->availableGeometry().bottomRight());
        quickOk &= quickPopup->isVisible() && !quickColors->isVisible() && screen()->availableGeometry().contains(quickPopup->frameGeometry());
        quickNumber->setValue(41);quickOk &= eraserSize==41 && thickness==13;
        QKeyEvent escapeQuick(QEvent::KeyPress,Qt::Key_Escape,Qt::NoModifier);QApplication::sendEvent(quickSlider,&escapeQuick);quickOk &= !quickPopup->isVisible();
        chooseTool(DrawingTool::Text);quickSettings();quickNumber->setValue(60);quickOk &= textSize==60 && quickSlider->maximum()==72;
        quickSettings();quickOk &= !quickPopup->isVisible();beginText({50,120});
        quickOk &= quickKey && !quickKey->isEnabled();quickSettings();quickOk &= !quickPopup->isVisible();
        QKeyEvent typeQ(QEvent::KeyPress,Qt::Key_Q,Qt::NoModifier,"q");QApplication::sendEvent(textEditor,&typeQ);quickOk &= textEditor->toPlainText()=="q";commitText(false);
        chooseTool(DrawingTool::Marker);sizeSlider->setValue(5);eraserSize=35;textSize=24;syncSizeUi();checkpoint("Q menu/color/size/context/closing/text typing",quickOk);
        openPopup(palettePopup); bool uiOk=palettePopup->isVisible();
        uiOk &= quickOk;
        palettePopup->grab().save(evidence+"/palette.png");palettePopup->hide();
        setInk(QColor("#70b5ff"));uiOk &= colorButton->accent==ink;
        openPopup(sizePopup);uiOk &= sizePopup->isVisible();
        sizeNumber->setValue(17);uiOk &= thickness==17 && sizeSlider->value()==17 && preview->diameter==17;
        sizePopup->grab().save(evidence+"/size-popup.png");sizePopup->hide();
        notify("Снимок скопирован · Ctrl+V — вставить");uiOk &= toast->isVisible();
        grab().save(evidence+"/ui-overview.png");toast->hide();toastTimer.stop();
        uiOk &= handle->geometry().bottom()<canvas().top();
        tool(false); sizeSlider->setValue(1); bool sizesOk=toolSize()==1;
        sizeSlider->setValue(50); sizesOk &= toolSize()==50;
        tool(true); sizeSlider->setValue(1); sizesOk &= toolSize()==1;
        sizeSlider->setValue(50); sizesOk &= toolSize()==50;
        tool(false); sizesOk &= toolSize()==50;
        sizeSlider->setValue(5); tool(true); sizesOk &= toolSize()==50;
        sizeSlider->setValue(35); tool(false);
        strokes={{{QPointF(40,80),QPointF(180,80)},Qt::red,8,false}};
        auto img=inkImage(); bool ok=uiOk && sizesOk && qAlpha(img.pixel(90*devicePixelRatioF(),80*devicePixelRatioF()))>0;
        checkpoint("existing UI and ink",ok);
        strokes.push_back({{QPointF(90,60),QPointF(90,100)},Qt::black,20,true});
        img=inkImage(); ok &= qAlpha(img.pixel(90*devicePixelRatioF(),80*devicePixelRatioF()))==0;
        undo(); ok &= strokes.size()==1; redo(); ok &= strokes.size()==2;
        resize(720,600); ok &= size()==QSize(720,600); ok &= grab().save(path); strokes.clear();redoStack.clear();updateHistoryButtons();
        checkpoint("existing eraser/history/resize",ok);
        auto mouse=[this](QEvent::Type type,QPointF pos,Qt::KeyboardModifiers mods=Qt::NoModifier){
            QMouseEvent event(type,pos,mapToGlobal(pos.toPoint()),type==QEvent::MouseMove?Qt::NoButton:Qt::LeftButton,type==QEvent::MouseButtonRelease?Qt::NoButton:Qt::LeftButton,mods);QApplication::sendEvent(this,&event);
        };
        chooseTool(DrawingTool::Arrow);
        mouse(QEvent::MouseButtonPress,{60,120});mouse(QEvent::MouseMove,{230,125},Qt::ShiftModifier);
        ok &= draft && qAbs(draft->points[0].y()-draft->points[1].y())<.01 && strokes.isEmpty();
        mouse(QEvent::MouseButtonRelease,{230,125},Qt::ShiftModifier);
        ok &= strokes.size()==1 && strokes.last().kind==StrokeKind::Arrow;
        img=inkImage();ok &= qAlpha(img.pixel(120*devicePixelRatioF(),120*devicePixelRatioF()))>0;
        checkpoint("arrow preview/shift/commit",ok);
        chooseTool(DrawingTool::Rectangle);
        mouse(QEvent::MouseButtonPress,{260,220});mouse(QEvent::MouseButtonRelease,{160,170},Qt::ShiftModifier);
        const auto square=QRectF(strokes.last().points[0],strokes.last().points[1]).normalized();
        ok &= square.width()==square.height() && square.left()==160 && square.top()==120;
        checkpoint("square normalization",ok);
        const int count=strokes.size();mouse(QEvent::MouseButtonPress,{300,250});mouse(QEvent::MouseMove,{400,320});
        QKeyEvent cancel(QEvent::KeyPress,Qt::Key_Escape,Qt::NoModifier);QApplication::sendEvent(this,&cancel);
        ok &= !draft && strokes.size()==count;
        mouse(QEvent::MouseButtonPress,{310,250});mouse(QEvent::MouseButtonRelease,{310,250});ok &= strokes.size()==count;
        checkpoint("cancel/empty shape",ok);
        const int previousWidth=toolSize();
        QWheelEvent wheel(QPointF(100,100),mapToGlobal(QPoint(100,100)),QPoint(),QPoint(0,120),Qt::NoButton,Qt::ControlModifier,Qt::NoScrollPhase,false);QApplication::sendEvent(this,&wheel);
        ok &= toolSize()==previousWidth+1;sizeSlider->setValue(previousWidth);
        QKeyEvent spacePress(QEvent::KeyPress,Qt::Key_Space,Qt::NoModifier);QApplication::sendEvent(this,&spacePress);ok &= spaceHeld;
        QKeyEvent spaceRelease(QEvent::KeyRelease,Qt::Key_Space,Qt::NoModifier);QApplication::sendEvent(this,&spaceRelease);ok &= !spaceHeld;
        checkpoint("quick size/space state",ok);
        chooseTool(DrawingTool::Text);beginText({50,300});
        ok &= textEditor && !toolShortcuts.first()->isEnabled();
        QKeyEvent letters(QEvent::KeyPress,Qt::Key_unknown,Qt::NoModifier,QString::fromUtf8("Текст Bear G []"));QApplication::sendEvent(textEditor,&letters);
        QKeyEvent newline(QEvent::KeyPress,Qt::Key_Return,Qt::ShiftModifier,"\n");QApplication::sendEvent(textEditor,&newline);
        QKeyEvent more(QEvent::KeyPress,Qt::Key_unknown,Qt::NoModifier,QString::fromUtf8("Новая строка"));QApplication::sendEvent(textEditor,&more);
        QKeyEvent enter(QEvent::KeyPress,Qt::Key_Return,Qt::NoModifier);QApplication::sendEvent(textEditor,&enter);
        ok &= !textEditor && toolShortcuts.first()->isEnabled() && strokes.size()==count+1 && strokes.last().kind==StrokeKind::Text && strokes.last().text.contains('\n');
        checkpoint("inline text input/enter",ok);
        undo();ok &= strokes.size()==count;redo();ok &= strokes.size()==count+1;
        beginText({50,400});textEditor->setPlainText("cancel me");QApplication::sendEvent(textEditor,&cancel);ok &= !textEditor && strokes.size()==count+1;
        checkpoint("text undo/redo/cancel",ok);
        tool(true);mouse(QEvent::MouseButtonPress,{100,120});mouse(QEvent::MouseButtonRelease,{100,120});
        img=inkImage();ok &= qAlpha(img.pixel(100*devicePixelRatioF(),120*devicePixelRatioF()))==0;
        undo();img=inkImage();ok &= qAlpha(img.pixel(100*devicePixelRatioF(),120*devicePixelRatioF()))>0;
        checkpoint("shape eraser/undo",ok);
        if(!solidGlass)toggleGlass();toast->hide();
        ok &= solidGlass && glassButton->isChecked();
        ok &= grab().save(evidence+"/solid-board.png");
        toggleGlass();toast->hide();chooseTool(DrawingTool::Arrow);ok &= grab().save(evidence+"/transparent-board.png");
        // A larger illustrated overview makes visual review independent of the desktop background.
        strokes.clear();redoStack.clear();resize(1000,680);setInk(QColor("#ffcf56"));
        strokes.push_back({{{40,90},{660,90}},ink,30,false,StrokeKind::Text,"GlassBoard · Быстрые заметки"});
        strokes.push_back({{{60,180},{320,310}},QColor("#79d5aa"),4,false,StrokeKind::Rectangle});
        strokes.push_back({{{90,220},{310,220}},ink,5,false,StrokeKind::Arrow});
        strokes.push_back({{{390,180},{750,180}},QColor("#e4edf3"),22,false,StrokeKind::Text,"A — стрелка\nR — прямоугольник\nT — текст\nG — стекло / доска"});
        solidGlass=true;glassButton->setChecked(true);updateHistoryButtons();toast->hide();grab().save(evidence+"/tools-overview.png");
        strokes.clear();redoStack.clear();solidGlass=false;glassButton->setChecked(false);tool(false);updateHistoryButtons();update();return ok;
    }
    void smokeCapture(const QString &path, std::function<void(bool)> done) {
        auto *previous=new QMimeData;
        const auto *data=QApplication::clipboard()->mimeData();
        if(data) for(const auto &format:data->formats()) previous->setData(format,data->data(format));
        QApplication::clipboard()->setText("GlassBoard-save-clipboard-sentinel");
        screenshot(false,[this,previous,path,done](const QImage &saved){
            const bool saveOk=QApplication::clipboard()->text()=="GlassBoard-save-clipboard-sentinel"
                && QImage(path).convertToFormat(QImage::Format_RGB32)==saved.convertToFormat(QImage::Format_RGB32);
            screenshot(true,[this,previous,saveOk,done](const QImage &shot){
            const QImage copied=QApplication::clipboard()->image();
            bool ok=saveOk && !copied.isNull() && copied.size()==shot.size()
                && copied.convertToFormat(QImage::Format_RGB32)==shot.convertToFormat(QImage::Format_RGB32);
            if(!solidGlass)toggleGlass();
            screenshot(true,[this,previous,ok,done](const QImage &solidShot){
                const bool solidOk=solidShot.pixelColor(10,10)==boardColor && solidShot.pixelColor(solidShot.width()-10,solidShot.height()-10)==boardColor;
                solidGlass=false;glassButton->setChecked(false);update();
                QApplication::clipboard()->setMimeData(previous);done(ok && solidOk);
            });
            });
        },path);
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
