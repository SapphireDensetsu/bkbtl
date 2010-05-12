#include "stdafx.h"
#include <QtGui>
#include "qscreen.h"
#include "Emulator.h"

QScreen::QScreen(QWidget *parent) :
    QWidget(parent)
{
    setMinimumSize(BK_SCREEN_WIDTH, BK_SCREEN_HEIGHT);
    setFocusPolicy(Qt::StrongFocus);

    m_image = new QImage(BK_SCREEN_WIDTH, BK_SCREEN_HEIGHT, QImage::Format_RGB32);
}

QScreen::~QScreen()
{
    delete m_image;
}

void QScreen::saveScreenshot(QString strFileName)
{
    m_image->save(strFileName, _T("PNG"), -1);
}

void QScreen::paintEvent(QPaintEvent *event)
{
    Emulator_PrepareScreenRGB32(m_image->bits(), BlackWhiteScreen);

    // Center image
    m_nImageLeft = (this->width() - BK_SCREEN_WIDTH) / 2;
    m_nImageTop = (this->height() - BK_SCREEN_HEIGHT) / 2;

    QPainter painter(this);
    painter.drawImage(m_nImageLeft, m_nImageTop, *m_image);
}

void QScreen::keyPressEvent(QKeyEvent *event)
{
    if (! g_okEmulatorRunning) return;
    if (event->isAutoRepeat()) return;

    unsigned char bkscan = TranslateQtKeyToBkKey(event->key());
    if (bkscan == 0) return;

    Emulator_KeyEvent(bkscan, TRUE);
    event->accept();
}

void QScreen::keyReleaseEvent(QKeyEvent *event)
{
    unsigned char bkscan = TranslateQtKeyToBkKey(event->key());
    if (bkscan == 0) return;

    Emulator_KeyEvent(bkscan, FALSE);
    event->accept();
}

const unsigned char arrPcscan2BkscanLat[256] = {  // ���
/*       0     1     2     3     4     5     6     7     8     9     a     b     c     d     e     f  */
/*0*/    0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000,
/*1*/    0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000,
/*2*/    0040, 0000, 0000, 0016, 0017, 0010, 0032, 0031, 0033, 0000, 0000, 0000, 0000, 0023, 0000, 0000,
/*3*/    0060, 0061, 0062, 0063, 0064, 0065, 0066, 0067, 0070, 0071, 0000, 0000, 0000, 0000, 0000, 0000,
/*4*/    0000, 0101, 0102, 0103, 0104, 0105, 0106, 0107, 0110, 0111, 0112, 0113, 0114, 0115, 0116, 0117,
/*5*/    0120, 0121, 0122, 0123, 0124, 0125, 0126, 0127, 0130, 0131, 0132, 0000, 0000, 0000, 0000, 0000,
/*6*/    0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000,
/*7*/    0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000,
/*8*/    0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000,
/*9*/    0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000,
/*a*/    0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000,
/*b*/    0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000,
/*c*/    0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000,
/*d*/    0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000,
/*e*/    0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000,
/*f*/    0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000,
};
const unsigned char arrPcscan2BkscanRus[256] = {  // ���
/*       0     1     2     3     4     5     6     7     8     9     a     b     c     d     e     f  */
/*0*/    0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000,
/*1*/    0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000,
/*2*/    0040, 0000, 0000, 0016, 0017, 0010, 0032, 0031, 0033, 0000, 0000, 0000, 0000, 0023, 0000, 0000,
/*3*/    0060, 0061, 0062, 0063, 0064, 0065, 0066, 0067, 0070, 0071, 0000, 0000, 0000, 0000, 0000, 0000,
/*4*/    0000, 0106, 0111, 0123, 0127, 0125, 0101, 0120, 0122, 0133, 0117, 0114, 0104, 0120, 0124, 0135,
/*5*/    0132, 0112, 0113, 0131, 0105, 0107, 0115, 0103, 0136, 0116, 0121, 0000, 0000, 0000, 0000, 0000,
/*6*/    0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000,
/*7*/    0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000,
/*8*/    0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000,
/*9*/    0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000,
/*a*/    0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000,
/*b*/    0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000,
/*c*/    0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000,
/*d*/    0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000,
/*e*/    0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000,
/*f*/    0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000,
};


unsigned char QScreen::TranslateQtKeyToBkKey(int qtkey)
{
    switch (qtkey)
    {
    case Qt::Key_Down:      return 033;
    case Qt::Key_Up:        return 032;
    case Qt::Key_Left:      return 010;
    case Qt::Key_Right:     return 031;
    case Qt::Key_Enter:     return 012;
    case Qt::Key_Return:    return 012;
    case Qt::Key_Tab:       return 015;
    //case Qt::Key_Shift:     return 0;
    case Qt::Key_Space:     return 040;
    case Qt::Key_Backspace: return 030;
    //case Qt::Key_Control:   return 0;
    //case Qt::Key_F1:        return 0;
    //case Qt::Key_F2:        return 0;
    //case Qt::Key_F3:        return 0;
    //case Qt::Key_F4:        return 0;
    //case Qt::Key_F5:        return 0;
    //case Qt::Key_F7:        return 0;
    //case Qt::Key_F8:        return 0;
    }

    if (qtkey >= 32 && qtkey <= 255)
    {
        unsigned short bkregister = g_pBoard->GetKeyboardRegister();
        // �������� ������� �������� � ����������� �� ����� ���/��� � ��
        const unsigned char * pTable = ((bkregister & KEYB_LAT) == 0) ? arrPcscan2BkscanRus : arrPcscan2BkscanLat;
        return pTable[qtkey];
    }

    return 0;
}
