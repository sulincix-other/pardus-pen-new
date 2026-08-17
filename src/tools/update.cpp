#include <constants.h>
#include <widgets/DrawingWidget.h>
#include <widgets/WhiteBoard.h>
#include <widgets/OverView.h>
#include <widgets/Button.h>
#include <widgets/FloatingSettings.h>
#include <widgets/Background.h>

static int prev_active[5] = {-1, -1, -1, -1, -1};

void updateGui(){
    int pen = getPen();
    int lineStyle = drawing->getLineStyle();
    int penStyle = drawing->getPenStyle();
    int pageType = board->getType();
    int overlayType = board->getOverlayType();
    int active[5] = { pen, lineStyle, penStyle, pageType, overlayType };

    colorDialog->setVisible(!(pen == ERASER || pen == SELECTION));
    ov->setVisible(pen != SELECTION);
    thicknessSlider->setVisible(pen != SELECTION);
    thicknessLabel->setVisible(pen != SELECTION);
    modeDialog->setVisible(!(pen == ERASER || pen == SELECTION || pen == PENTEXT));
    penTypeDialog->setVisible(!(pen == ERASER || pen == SELECTION || pen == PENTEXT));

    toolButtons[PENMENU]->setStyleSheet("background-color: none;");
    toolButtons[ERASERMENU]->setStyleSheet("background-color: none;");

    if(pen == MARKER || pen == PEN || pen == PENTEXT || pen == SMART_PEN){
        set_icon(get_icon_by_id(pen), toolButtons[PENMENU]);
        toolButtons[PENMENU]->setStyleSheet("background-color:"+drawing->pen.color().name()+";");
    } else if (pen == ERASER){
        toolButtons[ERASERMENU]->setStyleSheet("background-color:"+drawing->pen.color().name()+";");
    } else {
        set_icon(get_icon_by_id(drawing->getLastPen()), toolButtons[PENMENU]);
    }

    // clear only previously-active buttons instead of all penButtons
    for (int i = 0; i < 5; i++) {
        if (prev_active[i] >= 0 && penButtons.contains(prev_active[i]) && penButtons[prev_active[i]] != nullptr) {
            penButtons[prev_active[i]]->setStyleSheet(QString("background-color: none;"));
        }
    }

    for(int i = 0; i < 5; i++){
        int btn = active[i];
        if(penButtons.contains(btn) && penButtons[btn] != nullptr){
            penButtons[btn]->setStyleSheet("background-color:"+drawing->pen.color().name()+";");
        }
        prev_active[i] = btn;
    }

    int value = drawing->penSize[pen];
    thicknessLabel->setText(QString(_("Size:")) + " " + QString::number(value));

    toolButtons[BACK]->setEnabled(drawing->isBackAvailable());
    toolButtons[NEXT]->setEnabled(drawing->isNextAvailable());
    toolButtons[PREVPAGE]->setEnabled(drawing->getPageNum() > 0);
    toolButtons[NEXTPAGE]->setEnabled(drawing->getPageNum() < drawing->max);
    pageLabel->setText(QString::number(drawing->getPageNum()));

    toolButtons[OVERLAYSCALEDOWN]->setEnabled(board->ratios[drawing->getPageNum()] >= 30);
    toolButtons[OVERLAYSCALEUP]->setEnabled(board->ratios[drawing->getPageNum()] <= 200);

    ov->updateImage();
    floatingSettings->reload();
}



