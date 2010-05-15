#include "stdafx.h"
#include <QtGui/QApplication>
#include <QtCore/QThread>
#include <QtCore/QTimer>
#include <QSettings>
#include <QDesktopWidget>
#include "main.h"
#include "mainwindow.h"
#include "Emulator.h"


void RestoreSettings();


QApplication *g_Application;
QMainWindow *g_MainWindow;
QSettings *g_Settings;


int main(int argc, char *argv[])
{
    QApplication application(argc, argv);
    g_Application = &application;

    QSettings settings(QSettings::IniFormat, QSettings::UserScope, _T("Back to Life"), _T("BKBTL"));
    g_Settings = &settings;

    MainWindow w;
    g_MainWindow = &w;

    // Center main window on the screen
    int xMainLeft = (g_Application->desktop()->width() - w.width()) / 2;
    if (xMainLeft < 0) xMainLeft = 0;
    int yMainTop = (g_Application->desktop()->height() - w.height()) / 2;
    if (yMainTop < 0) yMainTop = 0;
    w.move(xMainLeft, yMainTop);

    if (!Emulator_Init()) return 255;
    //if (!Emulator_InitConfiguration((BKConfiguration)Settings_GetConfiguration()))
    if (!Emulator_InitConfiguration(BK_CONF_BK0010_FOCAL))
        return 255;

    w.show();

    RestoreSettings();
    w.UpdateMenu();

    QTimer timerFrame;
    QObject::connect(&timerFrame, SIGNAL(timeout()), &w, SLOT(emulatorFrame()), Qt::AutoConnection);
    timerFrame.start(20);

    int result = application.exec();

    Emulator_Done();

    settings.sync();

    Common_Cleanup();

    return result;
}

QApplication* Global_getApplication()
{
    return g_Application;
}
QMainWindow* Global_getMainWindow()
{
    return g_MainWindow;
}
QSettings* Global_getSettings()
{
    return g_Settings;
}

void RestoreSettings()
{
    // Reattach floppy images
    for (int slot = 0; slot < 4; slot++)
    {
        //QString path = Settings_GetFloppyFilePath(slot);
        //if (path.length() > 0)
        //{
        //    if (! g_pBoard->AttachFloppyImage(slot, qPrintable(path)))
        //        Settings_SetFloppyFilePath(slot, NULL);
        //}
    }
}
