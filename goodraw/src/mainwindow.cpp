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
#include <QScreen>
#include <QGuiApplication>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QUrl>
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
    
    // Set window size to 80% of screen size
    QScreen* screen = QGuiApplication::primaryScreen();
    if (screen) {
        QRect screenGeometry = screen->availableGeometry();
        int width = static_cast<int>(screenGeometry.width() * 0.8);
        int height = static_cast<int>(screenGeometry.height() * 0.8);
        resize(width, height);
        
        // Center the window on screen
        int x = (screenGeometry.width() - width) / 2;
        int y = (screenGeometry.height() - height) / 2;
        move(x, y);
    }
    
    // Enable drag and drop functionality
    setAcceptDrops(true);
    
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

/**
 * @brief Handle drag enter events for file drag and drop
 * 
 * Accepts drag events that contain file URLs with supported RAW file extensions.
 * Provides visual feedback when dragging supported files over the window.
 * 
 * @param event Drag enter event containing MIME data with file information
 */
void MainWindow::dragEnterEvent(QDragEnterEvent* event) {
    // Check if the drag contains file URLs
    if (event->mimeData()->hasUrls()) {
        QList<QUrl> urls = event->mimeData()->urls();
        
        // Check if at least one file has a supported RAW extension
        for (const QUrl& url : urls) {
            if (url.isLocalFile()) {
                QString filePath = url.toLocalFile();
                QString extension = QFileInfo(filePath).suffix().toLower();
                
                // List of supported RAW file extensions
                QStringList supportedExtensions = {
                    "cr2", "cr3",           // Canon
                    "nef", "nrw",           // Nikon  
                    "arw", "srf", "sr2",    // Sony
                    "orf",                  // Olympus
                    "rw2",                  // Panasonic
                    "dng",                  // Adobe DNG
                    "raf",                  // Fujifilm
                    "pef", "ptx",           // Pentax
                    "x3f",                  // Sigma
                    "mrw",                  // Minolta
                    "dcr", "kdc",           // Kodak
                    "erf",                  // Epson
                    "mef",                  // Mamiya
                    "mos",                  // Leaf
                    "raw", "rwl"            // Generic
                };
                
                if (supportedExtensions.contains(extension)) {
                    event->acceptProposedAction();
                    return;
                }
            }
        }
    }
    
    // Reject the drag if no supported files found
    event->ignore();
}

/**
 * @brief Handle drop events for file drag and drop
 * 
 * Processes dropped files and loads the first supported RAW file found.
 * Shows error message if no supported files are dropped.
 * 
 * @param event Drop event containing dropped file paths
 */
void MainWindow::dropEvent(QDropEvent* event) {
    if (event->mimeData()->hasUrls()) {
        QList<QUrl> urls = event->mimeData()->urls();
        
        // Process the first supported RAW file found
        for (const QUrl& url : urls) {
            if (url.isLocalFile()) {
                QString filePath = url.toLocalFile();
                QString extension = QFileInfo(filePath).suffix().toLower();
                
                // Same supported extensions list as in dragEnterEvent
                QStringList supportedExtensions = {
                    "cr2", "cr3", "nef", "nrw", "arw", "srf", "sr2", "orf",
                    "rw2", "dng", "raf", "pef", "ptx", "x3f", "mrw", 
                    "dcr", "kdc", "erf", "mef", "mos", "raw", "rwl"
                };
                
                if (supportedExtensions.contains(extension)) {
                    // Load the RAW file
                    loadRaw(filePath);
                    event->acceptProposedAction();
                    return;
                }
            }
        }
        
        // Show error if no supported files were found
        QMessageBox::warning(this, "Unsupported File", 
                           "Please drop a supported RAW file format.\n"
                           "Supported formats: CR2, CR3, NEF, ARW, DNG, RAF, and others.");
    }
    
    event->ignore();
}
