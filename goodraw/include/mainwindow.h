// mainwindow.h
#pragma once
#include <QMainWindow>
#include <QSlider>
#include "glimagewidget.h"

class MainWindow : public QMainWindow{
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent=nullptr);
    void loadRaw(const QString& path);

private:
    GLImageWidget* glWidget;
    QSlider* exposureSlider;
    QSlider* kelvinSlider; // Now: temperature shift slider
    QSlider* contrastSlider;
    float camMulR = 1.0f, camMulG = 1.0f, camMulB = 1.0f; // Store image WB
    QString lastOpenedDirectory; // Remember last directory for file dialog
};
