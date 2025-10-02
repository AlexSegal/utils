// mainwindow.h
#pragma once
#include <QMainWindow>
#include <QSlider>
#include <QDragEnterEvent>
#include <QDropEvent>
#include "glimagewidget.h"
#include "halfimage.h"
#include "rawdecoder.h"

/**
 * @brief Main application window with image viewer and adjustment controls
 * 
 * Provides the primary interface for GoodRAW with File menu, adjustment sliders,
 * and OpenGL image display. Manages RAW file loading and white balance calculations.
 */
class MainWindow : public QMainWindow{
    Q_OBJECT
public:
    /**
     * @brief Construct main window with UI controls and GL viewer
     * @param parent Parent widget (optional)
     */
    explicit MainWindow(QWidget* parent=nullptr);
    
    /**
     * @brief Load and display a RAW image file
     * @param path Absolute path to RAW image file (.cr2, .nef, .arw, etc.)
     */
    void loadRaw(const QString& path);

protected:
    /**
     * @brief Handle drag enter events for file drag and drop
     * @param event Drag enter event containing file information
     */
    void dragEnterEvent(QDragEnterEvent* event) override;
    
    /**
     * @brief Handle drop events for file drag and drop
     * @param event Drop event containing dropped file paths
     */
    void dropEvent(QDropEvent* event) override;

private:
    GLImageWidget* glWidget;              ///< OpenGL image display widget
    QSlider* exposureSlider;              ///< Exposure adjustment in stops (-3 to +3)
    QSlider* kelvinSlider;                ///< White balance temperature shift
    QSlider* contrastSlider;              ///< Contrast adjustment (0.5 to 2.0)
    float camMulR = 1.0f, camMulG = 1.0f, camMulB = 1.0f; ///< Camera white balance multipliers
    QString lastOpenedDirectory;         ///< Remember last directory for file dialog
};
