// mainwindow.cpp
#include "mainwindow.h"
#include "rawdecoder.h"
#include "halfimage.h"
#include "colortransform.h"
#include <QVBoxLayout>
#include <QSlider>
#include <QFileDialog>
#include <QMessageBox>
#include <QMenuBar>
#include <QLabel>
#include <QFileInfo>
#include <tuple>


MainWindow::MainWindow(QWidget* parent):QMainWindow(parent)
{
    QWidget* central = new QWidget(this);
    setCentralWidget(central);

    // Main horizontal layout
    QHBoxLayout* mainLayout = new QHBoxLayout(central);

    // Image widget on the left
    glWidget = new GLImageWidget(this);
    mainLayout->addWidget(glWidget, 1); // stretch factor 1

    // Controls on the right
    QWidget* controlsWidget = new QWidget(this);
    QVBoxLayout* controlsLayout = new QVBoxLayout(controlsWidget);
    controlsLayout->setAlignment(Qt::AlignTop);

    // Helper to make slider+label+value
    auto makeLabeledSlider = [controlsLayout](const QString& labelText, int min, int max, int val) {
        QWidget* row = new QWidget;
        QHBoxLayout* rowLayout = new QHBoxLayout(row);
        rowLayout->setContentsMargins(0,0,0,0);
        QLabel* label = new QLabel(labelText);
        QSlider* slider = new QSlider(Qt::Horizontal);
        slider->setRange(min, max);
        slider->setValue(val);
        QLabel* valueLabel = new QLabel(QString::number(val));
        valueLabel->setFixedWidth(40);
        rowLayout->addWidget(label);
        rowLayout->addWidget(slider, 1);
        rowLayout->addWidget(valueLabel);
        row->setLayout(rowLayout);
        controlsLayout->addWidget(row);
        return std::make_tuple(slider, valueLabel);
    };

    auto [exposureSlider_, exposureValueLabel] = makeLabeledSlider("Exposure", -50, 50, 0);
    auto [kelvinSlider_, kelvinValueLabel] = makeLabeledSlider("Temp Shift", -100, 100, 0);
    auto [contrastSlider_, contrastValueLabel] = makeLabeledSlider("Contrast", 50, 200, 100);

    // Store sliders for later use
    exposureSlider = exposureSlider_;
    kelvinSlider = kelvinSlider_;
    contrastSlider = contrastSlider_;

    // Update value labels and glWidget on slider change
    auto connectSlider = [](QSlider* slider, QLabel* valueLabel, std::function<void()> onChange) {
        QObject::connect(slider, &QSlider::valueChanged, [slider, valueLabel, onChange](int v){
            valueLabel->setText(QString::number(v));
            onChange();
        });
    };

    auto updateGlWidget = [this]() {
        float exp = exposureSlider->value()/10.0f;
        int shift = kelvinSlider->value();
        float con = contrastSlider->value()/100.0f;
        // Relative WB shift: scale R/B, keep G fixed
        float alpha = 0.01f;
        float r = camMulR, g = camMulG, b = camMulB;
        if (shift > 0) {
            r *= (1.0f + alpha * shift);
            b /= (1.0f + alpha * shift);
        } else if (shift < 0) {
            r /= (1.0f - alpha * shift);
            b *= (1.0f - alpha * shift);
        }
        glWidget->setExposure(exp);
        glWidget->setWB(r, g, b);
        glWidget->setContrast(con);
    };

    connectSlider(exposureSlider, exposureValueLabel, updateGlWidget);
    connectSlider(kelvinSlider, kelvinValueLabel, updateGlWidget);
    connectSlider(contrastSlider, contrastValueLabel, updateGlWidget);

    // Add a stretch at the end to keep controls at the top
    controlsLayout->addStretch(1);
    mainLayout->addWidget(controlsWidget, 0); // stretch factor 0

    // File menu actions
    QMenu* fileMenu = menuBar()->addMenu("File");
    
    // Open action
    QAction* openAct = new QAction("Open...", this);
    openAct->setShortcut(QKeySequence::Open);
    connect(openAct, &QAction::triggered, [this]() {
        QString filename = QFileDialog::getOpenFileName(this,
                                                        "Open RAW File",
                                                        lastOpenedDirectory,
                                                        "RAW Files (*.cr2 *.cr3 *.nef *.arw *.dng *.raf *.orf *.rw2);;All Files (*.*)");
        if(!filename.isEmpty()) {
            try {
                loadRaw(filename);
            } catch (const std::exception& e) {
                QMessageBox::critical(this, "Error", QString("Failed to load RAW file:\n%1").arg(e.what()));
            }
        }
    });
    fileMenu->addAction(openAct);
    
    fileMenu->addSeparator();
    
    // Export action
    QAction* exportAct = new QAction("Export PNG", this);
    connect(exportAct, &QAction::triggered, [this]() {
        QString filename = QFileDialog::getSaveFileName(this,
                                                        "Save Image",
                                                        "",
                                                        "PNG (*.png);;JPEG (*.jpg)");
        if(!filename.isEmpty()) {
            if (glWidget->exportImage(filename))
                QMessageBox::information(this,"Export","Saved successfully!");
            else
                QMessageBox::warning(this,"Export","Failed to save image!");
        }
    });
    fileMenu->addAction(exportAct);
    
    // Set initial focus to the GL widget
    glWidget->setFocus();
}

void MainWindow::loadRaw(const QString& path) 
{
    fprintf(stderr, "loadRaw: Loading %s\n", path.toStdString().c_str());

    // Remember the directory for next open dialog
    QFileInfo fileInfo(path);
    lastOpenedDirectory = fileInfo.absolutePath();

    RawImageResult result = loadRawImage(path.toStdString());
    HalfImage img = convertLibRaw16ToHalf(result.image);
    cameraToACEScg(img, result.color);
    glWidget->setImage(img);
    
    // Ensure GL widget has focus for keyboard events
    glWidget->setFocus();

    // Store cam_mul for relative WB
    float r = result.color.cam_mul[0];
    float g = result.color.cam_mul[1];
    float b = result.color.cam_mul[2];
    if (g > 0.0f) {
        r /= g;
        b /= g;
        g = 1.0f;
    } else {
        r = g = b = 1.0f;
    }
    camMulR = r;
    camMulG = g;
    camMulB = b;
    kelvinSlider->setValue(0);
    // Apply initial WB using cam_mul
    // updateGlWidget is a lambda in the constructor, so we need to trigger the slider or call it directly
    // We'll trigger the slider to ensure UI updates
    QMetaObject::invokeMethod(kelvinSlider, "valueChanged", Q_ARG(int, 0));
}
