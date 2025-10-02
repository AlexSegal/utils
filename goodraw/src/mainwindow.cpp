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
#include <QSplitter>
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
#include <QDebug>
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
    // Create main splitter for resizable layout
    QSplitter* mainSplitter = new QSplitter(Qt::Horizontal, this);
    setCentralWidget(mainSplitter);
    
    // Apply comprehensive dark theme styling
    mainSplitter->setStyleSheet(
        // Main application dark theme
        "QMainWindow {"
        "    background-color: #2b2b2b;"
        "    color: #ffffff;"
        "}"
        
        // Splitter handle styling
        "QSplitter::handle {"
        "    background-color: #404040;"
        "    border: 1px solid #555555;"
        "    width: 3px;"
        "    margin: 0px;"
        "}"
        "QSplitter::handle:hover {"
        "    background-color: #606060;"
        "}"
        "QSplitter::handle:pressed {"
        "    background-color: #707070;"
        "}"
        
        // Widget backgrounds
        "QWidget {"
        "    background-color: #2b2b2b;"
        "    color: #ffffff;"
        "}"
        
        // Labels styling
        "QLabel {"
        "    color: #cccccc;"
        "    background-color: transparent;"
        "}"
        
        // Slider styling (reduced by ~15%)
        "QSlider::groove:horizontal {"
        "    border: 1px solid #404040;"
        "    height: 5px;"  // 6px * 0.85 ≈ 5px
        "    background-color: #1e1e1e;"
        "    border-radius: 2px;"  // 3px * 0.85 ≈ 2px
        "}"
        "QSlider::handle:horizontal {"
        "    background-color: #505050;"
        "    border: 1px solid #606060;"
        "    width: 12px;"   // 14px * 0.85 ≈ 12px
        "    height: 12px;"  // 14px * 0.85 ≈ 12px
        "    border-radius: 6px;"  // 7px * 0.85 ≈ 6px  
        "    margin: -4px 0;"  // -5px * 0.85 ≈ -4px
        "}"
        "QSlider::handle:horizontal:hover {"
        "    background-color: #606060;"
        "}"
        "QSlider::handle:horizontal:pressed {"
        "    background-color: #707070;"
        "}"
        "QSlider::sub-page:horizontal {"
        "    background-color: #404040;"
        "    border-radius: 2px;"  // 3px * 0.85 ≈ 2px
        "}"
        
        // Line edit (text field) styling (reduced by ~15%)
        "QLineEdit {"
        "    background-color: #1e1e1e;"
        "    border: 1px solid #404040;"
        "    border-radius: 2px;"  // 3px * 0.85 ≈ 2px
        "    padding: 1px 3px;"    // 2px * 0.85 ≈ 1px, 4px * 0.85 ≈ 3px
        "    color: #ffffff;"
        "    selection-background-color: #505050;"
        "}"
        "QLineEdit:focus {"
        "    border: 1px solid #606060;"
        "}"
        
        // Menu bar styling
        "QMenuBar {"
        "    background-color: #2b2b2b;"
        "    color: #ffffff;"
        "    border-bottom: 1px solid #404040;"
        "}"
        "QMenuBar::item {"
        "    background-color: transparent;"
        "    padding: 3px 7px;"  // 4px * 0.85 ≈ 3px, 8px * 0.85 ≈ 7px
        "}"
        "QMenuBar::item:selected {"
        "    background-color: #404040;"
        "}"
        "QMenuBar::item:pressed {"
        "    background-color: #505050;"
        "}"
        
        // Menu styling
        "QMenu {"
        "    background-color: #2b2b2b;"
        "    color: #ffffff;"
        "    border: 1px solid #404040;"
        "}"
        "QMenu::item {"
        "    padding: 3px 14px;"  // 4px * 0.85 ≈ 3px, 16px * 0.85 ≈ 14px
        "}"
        "QMenu::item:selected {"
        "    background-color: #505050;"
        "}"
    );
    
    // Set handle width for better visibility and usability
    mainSplitter->setHandleWidth(3);

    // Image widget on the left side of splitter
    glWidget = new GLImageWidget(this);
    mainSplitter->addWidget(glWidget);

    // Controls widget on the right side of splitter
    QWidget* controlsWidget = new QWidget(this);
    QVBoxLayout* controlsLayout = new QVBoxLayout(controlsWidget);
    controlsLayout->setAlignment(Qt::AlignTop);
    controlsLayout->setContentsMargins(8, 8, 8, 8); // Reduced padding by ~15% (10 * 0.85 ≈ 8)
    mainSplitter->addWidget(controlsWidget);
    
    // Set initial splitter proportions (~80% image, ~20% controls - 30% reduction on controls)
    mainSplitter->setStretchFactor(0, 8); // Image gets much more space
    mainSplitter->setStretchFactor(1, 2); // Controls get significantly less space
    
    // Set minimum sizes to prevent collapse (reduced controls minimum by ~30%)
    glWidget->setMinimumWidth(300);
    controlsWidget->setMinimumWidth(175); // 250 * 0.7 = 175

    // Create precision sliders for professional RAW adjustments
    
    // Exposure slider: -5.0 to +5.0 stops with 0.1 step precision
    exposureSlider = new PrecisionSlider(this);
    exposureSlider->setLabel("Exposure (stops):");
    exposureSlider->setRange(-5.0f, 5.0f);
    exposureSlider->setDefaultValue(0.0f);
    exposureSlider->setSingleStep(0.1f);
    exposureSlider->setPageStep(1.0f);
    exposureSlider->setDecimals(2);
    exposureSlider->setValue(0.0f);
    controlsLayout->addWidget(exposureSlider);
    
    // White balance temperature shift: -1000K to +1000K with 10K steps
    kelvinSlider = new PrecisionSlider(this);
    kelvinSlider->setLabel("Temperature (K):");
    kelvinSlider->setRange(-1000.0f, 1000.0f);
    kelvinSlider->setDefaultValue(0.0f);
    kelvinSlider->setSingleStep(10.0f);
    kelvinSlider->setPageStep(100.0f);
    kelvinSlider->setDecimals(0);
    kelvinSlider->setValue(0.0f);
    controlsLayout->addWidget(kelvinSlider);
    
    // Contrast slider: 0.1 to 3.0 with 0.05 step precision
    contrastSlider = new PrecisionSlider(this);
    contrastSlider->setLabel("Contrast:");
    contrastSlider->setRange(0.1f, 3.0f);
    contrastSlider->setDefaultValue(1.0f);
    contrastSlider->setSingleStep(0.05f);
    contrastSlider->setPageStep(0.5f);
    contrastSlider->setDecimals(2);
    contrastSlider->setValue(1.0f);
    controlsLayout->addWidget(contrastSlider);

    /**
     * @brief Lambda to update GL widget with current precision slider values
     * 
     * Uses direct float values from precision sliders and applies relative
     * white balance shifts while preserving camera calibration.
     */
    auto updateGlWidget = [this]() {
        float exp = exposureSlider->value();          // Direct exposure in stops
        float shift = kelvinSlider->value();          // Direct Kelvin shift
        float con = contrastSlider->value();          // Direct contrast multiplier
        
        // Relative WB shift: scale R/B channels, keep G fixed
        // Formula: 1% per 10K temperature shift
        float alpha = 0.0001f; // 0.01% per Kelvin
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

    // Connect precision slider signals for real-time updates
    connect(exposureSlider, &PrecisionSlider::valueChanged, updateGlWidget);
    connect(kelvinSlider, &PrecisionSlider::valueChanged, updateGlWidget);
    connect(contrastSlider, &PrecisionSlider::valueChanged, updateGlWidget);

    // Add a stretch at the end to keep controls at the top
    controlsLayout->addStretch(1);

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
    
    // Set exposure slider to camera's exposure compensation from EXIF
    // Direct float value in stops - no conversion needed with PrecisionSlider
    exposureSlider->setValue(result.exposureCompensation);
    qDebug() << "Applied camera exposure compensation:" << result.exposureCompensation << "stops";
    
    // Apply initial adjustments by triggering value changed signals
    exposureSlider->setValue(exposureSlider->value()); // Trigger exposure update
    kelvinSlider->setValue(0.0f);                      // Reset temperature to neutral
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
