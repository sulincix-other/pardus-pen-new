#include <widgets/DrawingWidget.h>
#include <widgets/WhiteBoard.h>
#include <widgets/Background.h>
#include <widgets/FloatingSettings.h>
#include <constants.h>
#include <utils/Storage.h>
#include <utils/Settings.h>
#include <utils/misc.h>
#include "utils/StrokeRecognition.h"

#ifdef QPRINTER
#include <QPrinter>
#endif
#ifdef LIBARCHIVE
#include <utils/Archive.h>
#endif
#include <stdio.h>

QString cache_main = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + "/pardus-pen/";
QString cache = cache_main + generateRandomString(5)+"/";

/*
penType:
 - 0 eraser
 - 1 pen
 - 2 marker
*/

#ifdef QT5
#define points touchPoints
#define position pos
#endif

int curEventButtons = 0;

DrawingWidget::DrawingWidget(QWidget *parent): QWidget(parent) {
    initializeImage(size());
    penSize[PEN] = get_int("pen-size");
    penSize[ERASER] = get_int("eraser-size");
    penSize[MARKER] = get_int("marker-size");
    penSize[PENTEXT] = get_int("pentype-size");
    penSize[SMART_PEN] = get_int("smart-pen-size");
    if(penSize[PENTEXT] == 0) penSize[PENTEXT] = 24;
    penType=PEN;
    penStyle=SPLINE;
    lineStyle=NORMAL;
    lastPenType = penType;
    setMouseTracking(true);
    setAttribute(Qt::WA_AcceptTouchEvents);
    num_of_press = 0;
    cropWidget = new MovableWidget(mainWidget);
    //cropWidget->stackUnder(this);
    QBoxLayout* cropLayout = new QVBoxLayout(cropWidget);
    cropLayout->addWidget(cropWidget->crop);
    cropLayout->setContentsMargins(0,0,0,0);
    cropLayout->setSpacing(0);
    cropWidget->setStyleSheet("border: 2px solid "+pen.color().name()+";");
    cropWidget->hide();

    //QScreen *screen = QGuiApplication::primaryScreen();
    fpressure = get_int((char*)"pressure") / 100.0;

    setFocusPolicy(Qt::StrongFocus);
    textActive = false;

    removeDirectory(cache);
    pen = QPen(QColor(get_string("color")), 0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
}

void DrawingWidget::addPoint(int id, QPointF data) {
    if(geo.size(id) == 0) {
        geo.saveValue(id, 0, data);
        return;
    }
    if((penType != SMART_PEN && lineStyle ==  NORMAL && penStyle == SPLINE) || penType == ERASER){
        geo.saveValue(id, 1, geo.load(id).loadValue(0));
        geo.saveValue(id, 0, data);
    } else {
        geo.addValue(id, data);
    }
}


void DrawingWidget::setOverlay(QImage img, int page){
    saveImageToFile(img, cache+QString::number(page));
}

QImage DrawingWidget::getOverlay(int page){
    return loadImageFromFile(cache+QString::number(page));
}

void DrawingWidget::addImage(QImage img){
    images.last_image_num++;
    images.image_count = images.last_image_num;
    images.saveValue(images.last_image_num, img.copy());
    updateGui();
}

void DrawingWidget::initializeImage(const QSize &size) {
    QPixmap pix = QPixmap(size*mainWidget->devicePixelRatio());
    pix.setDevicePixelRatio(mainWidget->devicePixelRatio());
    pix.fill(QColor("transparent"));
    image = pix;
}

void DrawingWidget::resizeEvent(QResizeEvent *event) {
    initializeImage(event->size());
    loadImage(images.last_image_num);
    QWidget::resizeEvent(event);
}

void DrawingWidget::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);
    QPainter p;
    p.begin(this);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setRenderHint(QPainter::SmoothPixmapTransform, true);
    p.drawPixmap(0, 0, image);
    p.end();
}


void DrawingWidget::clear() {
    image.fill(QColor("transparent"));
    images.clear();
    pages.saveValue(pages.last_page_num, images);
    update();
}

void DrawingWidget::clearAll() {
    images.clear();
    pages.clear();
    goPage(0);
    update();
}



static QPointF last_end = QPointF(0,0);
static QPointF last_begin = QPointF(0,0);

void DrawingWidget::selectionDraw(QPointF startPoint, QPointF endPoint) {
    image.fill(QColor("transparent"));
    painter.begin(&image);
    painter.setPen(Qt::NoPen);
    QColor penColor = pen.color();
    penColor.setAlpha(127);
    painter.setBrush(QBrush(penColor));
    painter.drawRect(QRectF(startPoint,endPoint));
    painter.end();
    update(QRectF(last_begin, last_end).toRect().normalized().adjusted(-1, -1, +1, +1));
    update(QRectF(startPoint, endPoint).toRect().normalized().adjusted(-1, -1, +1, +1));

    last_begin = startPoint;
    last_end = endPoint;
}

#ifdef LIBARCHIVE
void DrawingWidget::saveAll(QString file){
    if (!file.isEmpty()) {
        #ifdef QPRINTER
        if(file.endsWith(".pdf")){
            pages.savePdf(file);
            return;
        }
        #endif
        if(!file.endsWith(".pen")){
            file += ".pen";
        }
        pages.saveAll(file);
    }
}

void DrawingWidget::loadArchive(const QString& filename){
    pages.loadArchive(filename);
}
#endif
void DrawingWidget::loadImage(int num){
    QImage img = images.loadValue(num);
    if(img.isNull()){
        return;
    }
    int targetW = mainWidget->geometry().width();
    int targetH = mainWidget->geometry().height();
    if (img.size() != QSize(targetW, targetH)) {
        img = img.scaled(targetW, targetH);
    }
    QPainter p(&image);
    image.fill(QColor("transparent"));
    p.drawImage(QPointF(0,0), img);
    update();
}

void DrawingWidget::goNextPage(){
    goPage(pages.last_page_num+1);
}

void DrawingWidget::goPage(int num){
    int old = pages.last_page_num;
    images.overlayType = board->getOverlayType();
    images.pageType = board->getType();

    pages.saveValue(old, images);
    pages.last_page_num = num;

    images = pages.loadValue(pages.last_page_num);
    loadImage(images.last_image_num);
    board->backgrounds[old] = board->grab();
    board->backgrounds[old].setDevicePixelRatio(mainWidget->devicePixelRatio());

    board->setType(images.pageType);
    board->setOverlayType(images.overlayType);
    board->updateTransform();
    board->update();

}

void DrawingWidget::goPreviousPage(){
    goPage(pages.last_page_num-1);
}

void DrawingWidget::goPrevious(){
    if(penType == PENTEXT && textActive){
        commitText();
    }
    if(!isBackAvailable()){
        return;
    }
    images.last_image_num--;
    loadImage(images.last_image_num);
}


void DrawingWidget::goNext(){
    if(!isNextAvailable()){
        return;
    }
    images.last_image_num++;
    loadImage(images.last_image_num);
}

void DrawingWidget::setPen(int type){
    if(type != penType){
        if(penType == PENTEXT && textActive){
            commitText();
        }
        if(type != ERASER && type != SELECTION){
            lastPenType = type;
        }
        penType = type;
        QColor penColor = pen.color();
        penColor.setAlpha(255);
        if(type == MARKER){
            penColor.setAlpha(127);
        }
        pen.setColor(penColor);
    }
    if(type != SELECTION){
        cropWidget->hide();
    }
}

int DrawingWidget::getPen(){
    return penType;
}
int DrawingWidget::getLastPen(){
    return lastPenType;
}
void DrawingWidget::setPenStyle(int type){
    penStyle = type;
}

int DrawingWidget::getPenStyle(){
    return penStyle;
}

void DrawingWidget::setLineStyle(int type){
    if(type != lineStyle){
        lineStyle = type;
        switch(lineStyle){
            case NORMAL:
                pen.setStyle(Qt::SolidLine);
                break;
            case DOTLINE:
                pen.setStyle(Qt::DotLine);
                break;
            case LINELINE:
                pen.setStyle(Qt::DashLine);
                break;
            case FILLED:
                pen.setStyle(Qt::SolidLine);
                break;
        }
        if(penType == ERASER) {
            pen.setStyle(Qt::SolidLine);
        }
    }
}

int DrawingWidget::getLineStyle(){
    return lineStyle;
}

void DrawingWidget::eventHandler(int source, int type, int id, QPointF pos, float pressure){
    debug("source: %d type: %d id:%d pressure:%f\n", source, type, id, pressure);
    int ev_pen = penType;
    static bool eraserModeCached = false;
    static bool eraserMode = false;
    if (!eraserModeCached) {
        eraserMode = get_bool("eraser-mode");
        eraserModeCached = true;
    }
    if(eraserMode && ! is_etap){
        if(source & Qt::MiddleButton) {
            setPen(MARKER);
        } else if(source & Qt::RightButton) {
            setPen(ERASER);
        }
    }
    switch(type) {
        case PRESS:
            if (curs.drawing.contains(id) && curs.drawing[id]) {
                // pen type change during press
                // so release pen then repress
                if(curs.penType[id] != penType){
                    int new_pen = penType;
                    penType = curs.penType[id];
                    eventHandler(0, RELEASE, id, pos, pressure);
                    penType = new_pen;
                } else {
                    break;
                }
            }
            if(penType == PENTEXT) {
                if(floatingSettings->isVisible()){
                    floatingSettings->setHide();
                }
                commitText();
                textPos = pos;
                textBuffer.clear();
                textSavedState = image.copy();
                textActive = true;
                curs.penType[id] = penType;
                curs.drawing[id] = true;
                break;
            }
            curs.penType[id] = penType;
            curs.drawing[id] = true;
            if (num_of_press == 1 || id == -1){
                mergeSelection();
                if(penType != ERASER){
                    background->image = image.toImage();
                    background->update();
                    image.fill(QColor("transparent"));
                }
            }
            if(floatingSettings->isVisible()){
                floatingSettings->setHide();
            }
            geo.clear(id);
            addPoint(id, pos);
            if(penType == SELECTION) {
                break;
            }
            curs.setPosition(id, pos);
            curEventButtons = source;
            break;
        case MOVE:
            if (!curs.drawing.contains(id) || !curs.drawing[id]) {
                break;
            }
            switch(penType) {
                case SELECTION:
                    selectionDraw(geo.first(id), pos);
                    break;
                case PENTEXT:
                    break;
                default:
                    if(penType == ERASER) {
                        curs.setPosition(id, pos);
                        curs.setCursor(id, penSize[penType]);
                    }
                    addPoint(id, pos);
                    painter.begin(&image);
                    drawFunc(id, pressure);
                    painter.end();
                    break;
            }
            break;
        case RELEASE:
            if (!curs.drawing.contains(id) || !curs.drawing[id]) {
                break;
            }
            if(penType == SMART_PEN && penStyle == SPLINE) {
                canvas_backup = background->image.copy();
            }

            curs.drawing[id] = false;
            curs.hide(id);
            if(penType == PENTEXT) {
                break;
            }

            if(penType != ERASER && geo.size(id) < 2 && penType != SELECTION) {
                int fpenStype = penStyle;
                penStyle = LINE;
                addPoint(id, pos+QPointF(0,1));
                painter.begin(&image);
                drawLineToFunc(id, 1.0);
                painter.end();
                penStyle = fpenStype;
            }
            curEventButtons = 0;

            if(num_of_press == 0 || id == -1) {
                if(penType == SMART_PEN && penStyle == SPLINE){
                    bool e = false; // for first time clear
                    for (auto it = geo.values.begin(); it != geo.values.end(); ++it) {
                        int decision = performStrokeRecognition(it.key());
                        bool recognitionSuccessful =
                            decision != RECOG_UNKNOWN &&
                            decision != RECOG_LENGTH_ERROR &&
                            decision != RECOG_START_ERROR;
                        if(recognitionSuccessful){
                            if(!e){
                                background->applyImage(image.toImage());
                                addImage(background->image);
                                background->image = canvas_backup;
                                image.fill(QColor("transparent"));
                                e = true;
                            }
                            drawRecognizedShape(
                                decision,
                                recognitionVariables,
                                recognitionResult
                            );
                        }
                    }
                }
                curs.clear();

                if(penType == SELECTION) {
                    addPoint(id, pos);
                    createSelection(id);
                    update();
                }
                if(penType != ERASER){
                    background->applyImage(image.toImage());
                    image = QPixmap::fromImage(background->image);
                    background->image.fill(QColor("transparent"));
                }
                if(penType == SELECTION) {
                    break;
                }
                geo.clearAll();
                addImage(image.toImage());
            }
            if(penType != ERASER && penStyle != SPLINE){
                update();
            }
            break;
    }
    setPen(ev_pen);
}

#define DEV_IDLE 0
#define DEV_MOUSE 1
#define DEV_TABLET 2

static int last_device = 0;
bool DrawingWidget::event(QEvent *ev) {
    switch (ev->type()) {
        case QEvent::TouchBegin:
        case QEvent::TouchEnd:
        case QEvent::TouchUpdate: {
            if(last_device != DEV_MOUSE && last_device != DEV_IDLE) {
                break;
            }
            QTouchEvent *touchEvent = static_cast<QTouchEvent*>(ev);
            QList<QTouchEvent::TouchPoint> touchPoints = touchEvent->points();
            for(auto it = curs.images.begin(); it != curs.images.end(); it++){
                curs.hide(it.key());
            }
            int touch_cnt=0;
            foreach(const QTouchEvent::TouchPoint &touchPoint, touchPoints) {
                if ((Qt::TouchPointState)touchPoint.state() != Qt::TouchPointReleased) {
                    touch_cnt++;
                }
            }
            num_of_press = touch_cnt;
            if(num_of_press == 0){
                last_device = DEV_IDLE;
            } else {
                last_device = DEV_MOUSE;
            }
            foreach(const QTouchEvent::TouchPoint &touchPoint, touchPoints) {
                QPointF pos = touchPoint.position();
                if ((Qt::TouchPointState)touchPoint.state() == Qt::TouchPointPressed) {
                    eventHandler(Qt::LeftButton, PRESS, touchPoint.id(), pos, touchPoint.pressure());
                } else if ((Qt::TouchPointState)touchPoint.state() == Qt::TouchPointReleased) {
                    eventHandler(Qt::LeftButton, RELEASE, touchPoint.id(), pos, touchPoint.pressure());
                } else {
                    eventHandler(Qt::LeftButton, MOVE, touchPoint.id(), pos, touchPoint.pressure());
                }
            }
            return true;
        }
        case QEvent::TabletPress: {
            if(last_device != DEV_IDLE){
                break;
            }
            last_device = DEV_TABLET;
            num_of_press=1;
            QTabletEvent *tabletEvent = static_cast<QTabletEvent*>(ev);
            eventHandler(tabletEvent->buttons(), PRESS, -2, tabletEvent->position(), tabletEvent->pressure());
            return true;
        }
        case QEvent::TabletRelease: {
            if(last_device != DEV_TABLET){
                break;
            }
            last_device = DEV_IDLE;
            num_of_press=0;
            QTabletEvent *tabletEvent = static_cast<QTabletEvent*>(ev);
            eventHandler(tabletEvent->buttons(), RELEASE, -2, tabletEvent->position(), tabletEvent->pressure());
            return true;
        }
        case QEvent::TabletMove: {
            if(last_device != DEV_TABLET) {
                break;
            }
            QTabletEvent *tabletEvent = static_cast<QTabletEvent*>(ev);
            eventHandler(tabletEvent->buttons(), MOVE, -2, tabletEvent->position(), tabletEvent->pressure());
            return true;
        }
        case QEvent::MouseButtonPress: {
            if(last_device != DEV_IDLE){
                break;
            }
            last_device = DEV_MOUSE;
            QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(ev);
            eventHandler(mouseEvent->buttons(), PRESS, -1, mouseEvent->position(), 1.0);
            return true;
        }
        case QEvent::MouseButtonRelease: {
            if(last_device != DEV_MOUSE){
                break;
            }
            last_device = DEV_IDLE;
            QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(ev);
            eventHandler(mouseEvent->buttons(), RELEASE, -1, mouseEvent->position(), 1.0);
            return true;
        }
        case QEvent::MouseMove: {
            if(last_device != DEV_MOUSE) {
                break;
            }
            QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(ev);
            eventHandler(mouseEvent->buttons(), MOVE, -1, mouseEvent->position(), 1.0);
            return true;
        }

        default:
            break;
    }
    return QWidget::event(ev);
}

int DrawingWidget::getPageNum(){
    return pages.last_page_num;
}

bool DrawingWidget::isBackAvailable(){
    debug("last_page_num: %d image_count: %d\n", images.last_image_num, images.image_count );
    return images.last_image_num > images.removed +1;
}

bool DrawingWidget::isNextAvailable(){
    debug("last_page_num: %d image_count: %d\n", images.last_image_num, images.image_count );
    return images.last_image_num < images.image_count;
}

bool DrawingWidget::isPenType() {
    return penType == PEN ||
           penType == MARKER ||
           penType == PENTEXT ||
           penType == SMART_PEN;
}

QColor convertColor(const QColor& color) {
    int tot =  color.red() + color.blue() + color.green();
    if (tot > 382) {
        return QColor(0,0,0, color.alpha());
    } else {
        return QColor(255, 255, 255, color.alpha());

    }
}

void removeDirectory(const QString &path) {
    QDir dir(path);
    if (!dir.exists())
        return;

    QFileInfoList fileInfoList = dir.entryInfoList(QDir::NoDotAndDotDot | QDir::AllEntries | QDir::Hidden);
    foreach (const QFileInfo &fi, fileInfoList) {
        if (fi.isDir()) {
            removeDirectory(fi.absoluteFilePath());
        } else {
            QFile::remove(fi.absoluteFilePath());
        }
    }

    dir.rmdir(path);
}

void qImageToFile(const QImage& image, const QString& filename) {
    QFileInfo fileInfo(filename);
    QString dirname = fileInfo.dir().absolutePath();
    QDir dir(dirname);
    if (!dir.exists(dirname)) {
        dir.mkpath(dirname);
    }
    QPixmap pixmap = QPixmap::fromImage(image);
      if (pixmap.save(filename, "PNG")) {
            debug("Image saved successfully as %s\n", filename.toStdString().c_str());
        } else {
            debug("Failed to save image at %s\n",filename.toStdString().c_str());
        }
}

QString generateRandomString(int length) {
    const QString characters = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    QString randomString;

    QRandomGenerator *generator = QRandomGenerator::global();
    for (int i = 0; i < length; ++i) {
        int index = generator->bounded(characters.length());
        randomString.append(characters[index]);
    }

    return randomString;
}

void DrawingWidget::commitText() {
    if(!textActive || textBuffer.isEmpty()) return;
    textActive = false;
    addImage(image.toImage());
    textBuffer.clear();
}

void DrawingWidget::keyPressEvent(QKeyEvent *event) {
    if(penType != PENTEXT || !textActive) {
        QWidget::keyPressEvent(event);
        return;
    }

    QString text = event->text();
    QFont font("sans-serif", penSize[PENTEXT]);
    font.setStyleHint(QFont::SansSerif);

    if(event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        textBuffer += '\n';
    } else if(event->key() == Qt::Key_Backspace) {
        if(textBuffer.isEmpty()) return;
        textBuffer.chop(1);
    } else if(event->key() == Qt::Key_Escape) {
        commitText();
        return;
    } else if(text.isEmpty() || !text[0].isPrint()) {
        return;
    } else {
        textBuffer += text;
    }

    image = textSavedState;
    painter.begin(&image);
    painter.setFont(font);
    painter.setPen(pen);
    painter.setRenderHint(QPainter::Antialiasing, true);
    QStringList lines = textBuffer.split('\n');
    QFontMetrics fm(font);
    int lineHeight = fm.height();
    int y = textPos.y() + fm.ascent() - lineHeight / 2;
    for(const QString &line : lines) {
        painter.drawText(QPointF(textPos.x(), y), line);
        y += lineHeight;
    }
    painter.end();
    update();
}

void clearCache(){
    debug("Clear cache:%s\n", cache_main.toStdString().c_str());
    removeDirectory(cache_main);
}
