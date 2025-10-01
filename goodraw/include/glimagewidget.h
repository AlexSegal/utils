#pragma once
#include <QOpenGLWidget>
#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QMouseEvent>
#include "halfimage.h"

/**
 * @brief Transform parameters for image cropping and rotation
 */
struct CropTransform {
    float centerX=0.0f, centerY=0.0f;  ///< Pan offset in normalized coordinates
    float width=1.0f, height=1.0f;     ///< Crop dimensions (currently unused)
    float rotation=0.0f;               ///< Rotation angle in radians
};

/**
 * @brief OpenGL-accelerated RAW image viewer with real-time processing
 * 
 * Displays RAW images using OpenGL shaders for real-time exposure, white balance,
 * and contrast adjustments. Supports interactive navigation with mouse/keyboard.
 */
class GLImageWidget : public QOpenGLWidget, protected QOpenGLFunctions_3_3_Core {
    Q_OBJECT
public:
    /**
     * @brief Construct GL image widget with keyboard focus enabled
     * @param parent Parent widget (optional)
     */
    explicit GLImageWidget(QWidget* parent=nullptr);
    
    /**
     * @brief Load and display a new RAW image
     * @param img Half-precision float image data in ACEScg color space
     */
    void setImage(const HalfImage& img);
    
    /**
     * @brief Set exposure adjustment in stops
     * @param e Exposure value in stops (0.0 = no adjustment)
     */
    void setExposure(float e);
    
    /**
     * @brief Set white balance multipliers
     * @param r Red channel multiplier
     * @param g Green channel multiplier (typically 1.0)
     * @param b Blue channel multiplier
     */
    void setWB(float r,float g,float b);
    
    /**
     * @brief Set contrast adjustment
     * @param c Contrast multiplier (1.0 = no adjustment)
     */
    void setContrast(float c);
    
    /**
     * @brief Set zoom level
     * @param z Zoom factor (1.0 = default size)
     */
    void setZoom(float z);
    
    /**
     * @brief Set pan offset
     * @param x Horizontal pan in normalized coordinates
     * @param y Vertical pan in normalized coordinates
     */
    void setPan(float x,float y);
    
    /**
     * @brief Export current view to image file
     * @param filename Output file path (PNG/JPEG based on extension)
     * @return true if export successful, false otherwise
     */
    bool exportImage(const QString& filename);

protected:
    /// Initialize OpenGL context, shaders, and vertex buffers
    void initializeGL() override;
    
    /// Handle widget resize and update aspect ratio
    void resizeGL(int w,int h) override;
    
    /// Render image with current transformations and adjustments
    void paintGL() override;
    
    /// Handle mouse wheel for zoom with cursor-centered scaling
    void wheelEvent(QWheelEvent* event) override;
    
    /// Handle mouse press for pan/rotate initiation and focus
    void mousePressEvent(QMouseEvent* event) override;
    
    /// Handle mouse drag for panning and rotation
    void mouseMoveEvent(QMouseEvent* event) override;
    
    /// Handle keyboard input (F key for fit-to-viewport)
    void keyPressEvent(QKeyEvent* event) override;

private:
    /**
     * @brief Calculate aspect ratio correction for proper image display
     * 
     * Computes scaling factors to fit image in viewport while maintaining
     * aspect ratio. Updates aspectScale member variable.
     */
    void updateAspectScale();
    
    /**
     * @brief Reset view to default zoom, center, and rotation
     * 
     * Sets zoom to 1.0, centers image, and clears rotation. Called on
     * image load and F key press.
     */
    void fitToViewport();
    
    QOpenGLShaderProgram program;         ///< Compiled vertex/fragment shaders
    QOpenGLTexture* _m_tex = nullptr;     ///< RGB16F texture containing image data
    HalfImage imgData;                    ///< Local copy of image data for aspect calculations
    float exposure = 0.0f;                ///< Exposure adjustment in stops
    QVector3D wb = {1.0f,1.0f,1.0f};      ///< Interactive white balance RGB multipliers
    float contrast = 1.0f;                ///< Contrast multiplier
    float zoom = 1.0f;                    ///< User zoom factor (combined with 2.0 base scale)
    float panX = 0.0f;                    ///< Legacy pan X (unused with matrix system)
    float panY = 0.0f;                    ///< Legacy pan Y (unused with matrix system)
    QPoint lastMousePos;                  ///< Previous mouse position for delta calculations
    CropTransform crop;                   ///< Current pan/rotation state
    bool rotating = false;                ///< True when right-mouse rotation is active
    bool showGrid = false;                ///< Show rotation grid overlay
    QVector2D aspectScale = {1.0f, 1.0f}; ///< Aspect ratio correction factors

    GLuint vao = 0;                       ///< Vertex Array Object for quad rendering
    GLuint vbo = 0;                       ///< Vertex Buffer Object for quad vertices
};
