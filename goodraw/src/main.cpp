#include <QApplication>
#include "mainwindow.h"

int main(int argc,char** argv){
    QApplication a(argc,argv);
    MainWindow w;
    w.show();
    if(argc>1) w.loadRaw(argv[1]);
    return a.exec();
}
