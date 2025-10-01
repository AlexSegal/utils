/**
 * @file mainwindow.cpp
 * @brief Main application window implementation with UI controls
 * 
 * Provides the primary interface for GoodRAW with File menu, adjustment sliders,
 * and OpenGL image display. Manages RAW file loading with LibRaw's sRGB output
 * and real-time white balance adjustments.
 */

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

/**
 * @brief Construct main window with UI controls and GL viewer
 * 
 * Creates the complete interface with image display on left and adjustment
 * controls on right. Sets up File menu with Open/Export actions and
 * configures real-time slider updates.
 * 
 * @param parent Parent widget (optional)
 */

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

    /**
     * @brief Helper lambda to create labeled slider with value display
     * @param labelText Display name for the slider
     * @param min Minimum slider value
     * @param max Maximum slider value  
     * @param val Initial slider value
     * @return Tuple containing slider and value label widgets
     */
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

    /**
     * @brief Helper lambda to connect slider value changes to UI updates
     * @param slider QSlider widget to monitor
     * @param valueLabel QLabel to update with current value
     * @param onChange Callback function for value changes
     */
    auto connectSlider = [](QSlider* slider, QLabel* valueLabel, std::function<void()> onChange) {
        QObject::connect(slider, &QSlider::valueChanged, [slider, valueLabel, onChange](int v){
            valueLabel->setText(QString::number(v));
            onChange();
        });
    };

    /**
     * @brief Lambda to update GL widget with current slider values
     * 
     * Converts slider values to appropriate ranges and applies relative
     * white balance shifts while preserving camera calibration.
     */
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

/**
 * @brief Load and display a RAW image file
 * 
 * Processes RAW file through complete pipeline: LibRaw decoding → half-float
 * conversion → ACES color space transformation → GL widget display.
 * Extracts and applies camera white balance, remembers directory for file dialog.
 * 
 * @param path Absolute path to RAW image file (.cr2, .nef, .arw, etc.)
 */
void MainWindow::loadRaw(const QString& path) 
{
    // Remember the directory for next open dialog
    QFileInfo fileInfo(path);
    lastOpenedDirectory = fileInfo.absolutePath();

    RawImageResult result = loadRawImage(path.toStdString());
    HalfImage img = convertLibRaw16ToHalf(result.image);
    
    // LibRaw outputs XYZ - convert to ACEScg for professional color pipeline
    xyzToACEScg(img);
    
    glWidget->setImage(img);
    
    // Ensure GL widget has focus for keyboard events
    glWidget->setFocus();

    // Initialize interactive white balance controls to neutral
    camMulR = 1.0f;
    camMulG = 1.0f;
    camMulB = 1.0f;
    kelvinSlider->setValue(0);
    
    // Apply initial neutral white balance to GPU
    QMetaObject::invokeMethod(kelvinSlider, "valueChanged", Q_ARG(int, 0));
}
