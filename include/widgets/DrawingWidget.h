#ifndef DRAWINGWIDGET_H
#define DRAWINGWIDGET_H

#include <QWidget>
#include <QImage>
#include <QPoint>
#include <QPainter>
#include <QMouseEvent>
#include <QApplication>
#include <QScreen>
#include <QRandomGenerator>
#include <iostream>

#include <QtWidgets>

#include <QImage>
#include <QPainter>
#include <QKeyEvent>

#ifndef DEBUG
#define debug(...)
#else
#include <cstdio>
#include "../utils/misc.h"
#ifdef __LP64__
#define debug printf("[%s:%ld]:", __func__, get_epoch() - _cur_time); _cur_time = get_epoch(); printf
#else
#define debug printf("[%s:%d]:", __func__, get_epoch() - _cur_time); _cur_time = get_epoch(); printf
#endif
#endif

#define PADDING 8*scale
#define butsize (40*scale + PADDING)

#include "../utils/Selection.h"
#include "../utils/Storage.h"
#include "FloatingSettings.h"
#include "../utils/StrokeRecognition.h"

#define PRESS 0
#define MOVE 1
#define RELEASE 2

class DrawingWidget;

extern DrawingWidget *drawing;
extern QString cache;
extern int history;
extern QWidget *mainWidget;
extern QMainWindow *tool;
extern QMainWindow *tool2;
extern QMainWindow *tool3;
extern QSlider *scrollHSlider;
extern QSlider *scrollVSlider;
extern bool is_wayland;
extern bool force_xwayland;
extern bool is_etap;
extern float scale;

void updateGui();
void setPen(int type);
int getPen();
int getLastPen();
void setPenStyle(int style);
void setLineStyle(int style);
void openFile(QString filename);
bool saveImageToFile(const QImage &image, const QString &imageFilePath);
QImage loadImageFromFile(const QString &imageFilePath);
void removeDirectory(const QString &path);
void setupWidgets();
void setupPenType();
void setupSaveLoad();
bool isPenType();

#ifdef QPRINTER
extern bool PDFMODE;
QImage getPdfImage(int num, float ratio);
void loadPdf(QString path);
void loadPdfFromData(QByteArray data);
#endif

class DrawingWidget : public QWidget {
public:
    explicit DrawingWidget(QWidget *parent = nullptr);

    QPen pen;
    QPixmap image;
    MovableWidget* cropWidget;
    int penSize[10];
    int max = 99;
    void initializeImage(const QSize &size);
    void goPrevious();
    void goPage(int i);
    void goNext();
    void goPreviousPage();
    void goNextPage();
    void clear();
    void clearAll();
    void drawRecognizedShape(int decision,
                             const StrokeVariables &variables,
                             const StrokeResult &result);
    int performStrokeRecognition(qint64 id);
#ifdef LIBARCHIVE
    void saveAll(QString filename);
    void loadArchive(const QString& filename);
#endif
    void syncPageType(int type);
    int getPageNum();
    bool isBackAvailable();
    bool isNextAvailable();
    bool isPenType();
    void loadImage(int num);
    void mergeSelection();
    void clearSelection();
    void addImage(QImage img);
    void setPen(int type);
    int getPen();
    int getLastPen();
    void setPenStyle(int type);
    int getPenStyle();
    void setLineStyle(int type);
    int getLineStyle();
    void drawArrow(QPainter& painter, QPointF start, QPointF end);
    void eventHandler(int source, int type, int id, QPointF pos, float pressure);
    void setOverlay(QImage img, int page);
    QImage getOverlay(int page);
    void commitText();
#ifdef QPRINTER
    QString pdfPath;
#endif
protected:
    float fpressure;
    int num_of_press;
    bool eraser;
    void resizeEvent(QResizeEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void updateCursorMouse(qint64 i, QPoint pos);
    void updateCursorMouse(qint64 i, QPointF pos);
    void createSelection(int source);
    void drawLineToFunc(qint64 id, qreal pressure);
    void keyPressEvent(QKeyEvent *event) override;
    void drawFunc(qint64 id, qreal pressure);
    void selectionDraw(QPointF startPoint, QPointF endPoint);
    void addPoint(int id, QPointF data);
    bool event(QEvent * ev) override;
    int penType;
    int penStyle;
    int lineStyle;
    int lastPenType;
    GeometryStorage geo;
    StrokeVariables recognitionVariables;
    StrokeResult recognitionResult;
    QPainter painter;
    QPointF textPos;
    QString textBuffer;
    QPixmap textSavedState;
    QImage canvas_backup;
    bool textActive;
};

QColor convertColor(const QColor& color);
void qImageToFile(const QImage& image, const QString& filename);
QString generateRandomString(int length);

#endif // DRAWINGWIDGET_H

