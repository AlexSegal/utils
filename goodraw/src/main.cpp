/**
 * @file main.cpp
 * @brief GoodRAW application entry point
 * 
 * Initializes Qt application and main window, supports command-line
 * RAW file loading for direct image opening.
 */

#include <QApplication>
#include <QPalette>
#include <QColor>
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
    
    // Set application-wide dark theme
    a.setStyle("Fusion"); // Use Fusion style for better dark theme support
    
    // Set dark color palette for the entire application
    QPalette darkPalette;
    darkPalette.setColor(QPalette::Window, QColor(43, 43, 43));
    darkPalette.setColor(QPalette::WindowText, QColor(255, 255, 255));
    darkPalette.setColor(QPalette::Base, QColor(30, 30, 30));
    darkPalette.setColor(QPalette::AlternateBase, QColor(64, 64, 64));
    darkPalette.setColor(QPalette::ToolTipBase, QColor(0, 0, 0));
    darkPalette.setColor(QPalette::ToolTipText, QColor(255, 255, 255));
    darkPalette.setColor(QPalette::Text, QColor(255, 255, 255));
    darkPalette.setColor(QPalette::Button, QColor(64, 64, 64));
    darkPalette.setColor(QPalette::ButtonText, QColor(255, 255, 255));
    darkPalette.setColor(QPalette::BrightText, QColor(255, 255, 255));
    darkPalette.setColor(QPalette::Link, QColor(128, 128, 128));
    darkPalette.setColor(QPalette::Highlight, QColor(80, 80, 80));
    darkPalette.setColor(QPalette::HighlightedText, QColor(255, 255, 255));
    a.setPalette(darkPalette);
    
    MainWindow w;
    w.show();
    if(argc>1) w.loadRaw(argv[1]);
    return a.exec();
}
