/**
 * @file main.cpp
 * @brief GoodRAW application entry point
 * 
 * Initializes Qt application and main window, supports command-line
 * RAW file loading for direct image opening.
 */

#include <QApplication>
#include "mainwindow.h"

/**
 * @brief Main application entry point
 * 
 * Creates QApplication instance, shows main window, and optionally
 * loads RAW file from command line argument.
 * 
 * @param argc Argument count
 * @param argv Argument vector (argv[1] = optional RAW file path)
 * @return Application exit code
 */
int main(int argc,char** argv){
    QApplication a(argc,argv);
    MainWindow w;
    w.show();
    if(argc>1) w.loadRaw(argv[1]);
    return a.exec();
}
