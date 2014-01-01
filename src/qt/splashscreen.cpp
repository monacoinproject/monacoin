#include "splashscreen.h"
#include "clientversion.h"
#include "util.h"

#include <QPainter>
#undef loop /* ugh, remove this when the #define loop is gone from util.h */
#include <QApplication>

SplashScreen::SplashScreen(const QPixmap &pixmap, Qt::WindowFlags f) :
    QSplashScreen(pixmap, f)
{
    // set reference point, paddings
    int paddingLeftCol2         = 250;
    int paddingTopCol2          = 376;
    int line1 = 0;
    int line2 = 13;
    int line3 = 26;
    int line4 = 39;

    float fontFactor            = 1.0;

    // define text to place
    QString titleText       = QString(QApplication::applicationName()).replace(QString("-testnet"), QString(""), Qt::CaseSensitive); // cut of testnet, place it as single object further down
    QString versionText     = QString("Version %1 ").arg(QString::fromStdString(FormatFullVersion()));
    QString copyrightText1   = QChar(0xA9)+QString(" 2009-%1 ").arg(COPYRIGHT_YEAR) + QString(tr("The Bitcoin developers"));
    QString copyrightText2   = QChar(0xA9)+QString(" 2011-%1 ").arg(COPYRIGHT_YEAR) + QString(tr("The Litecoin developers"));
    QString copyrightText3   = QChar(0xA9)+QString(" 2013-%1 ").arg(COPYRIGHT_YEAR) + QString(tr("The Monacoin developers"));

    QString font            = "Arial";

   QString splash_filepath[] = {QString(":/images/splash_1"), QString(":/images/splash_2"), QString(":/images/splash_3"), QString(":/images/splash_4")};

    // load the bitmap for writing some text over it
    QPixmap newPixmap;
/*
    if(GetBoolArg("-testnet")) {
        newPixmap     = QPixmap(":/images/splash_testnet");
    }
    else {
        newPixmap     = QPixmap(":/images/splash2");
    }
*/
    time_t timer;
    time(&timer);
    int index = timer & 0x03;
    newPixmap = (splash_filepath[index]);


    QPainter pixPaint(&newPixmap);
    pixPaint.setPen(QColor(70,70,70));

    pixPaint.setFont(QFont(font, 9*fontFactor));
    pixPaint.drawText(paddingLeftCol2,paddingTopCol2+line4,versionText);

    // draw copyright stuff
    pixPaint.setFont(QFont(font, 9*fontFactor));
    pixPaint.drawText(paddingLeftCol2,paddingTopCol2+line1,copyrightText1);
    pixPaint.drawText(paddingLeftCol2,paddingTopCol2+line2,copyrightText2);
    pixPaint.drawText(paddingLeftCol2,paddingTopCol2+line3,copyrightText3);

    pixPaint.end();

    this->setPixmap(newPixmap);
}
