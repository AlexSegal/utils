#pragma once
#include <QOpenGLWidget>
#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QMouseEvent>
#include "halfimage.h"

struct CropTransform {
    float centerX=0.5f, centerY=0.5f;
    float width=1.0f, height=1.0f;
    float rotation=0.0f;
};

class GLImageWidget : public QOpenGLWidget, protected QOpenGLFunctions_3_3_Core {
    Q_OBJECT
public:
    explicit GLImageWidget(QWidget* parent=nullptr);
    void setImage(const HalfImage& img);
    void setExposure(float e);
    void setWB(float r,float g,float b);
    void setContrast(float c);
    void setZoom(float z);
    void setPan(float x,float y);
    bool exportImage(const QString& filename);

protected:
    void initializeGL() override;
    void resizeGL(int w,int h) override;
    void paintGL() override;
    void wheelEvent(QWheelEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;

private:
    void updateAspectScale(); // Calculate aspect ratio correction
    
    QOpenGLShaderProgram program;
    QOpenGLTexture* _m_tex = nullptr;
    HalfImage imgData;
    float exposure = 0.0f;
    QVector3D wb = {1.0f,1.0f,1.0f};
    float contrast = 1.0f;
    float zoom = 1.0f;
    float panX = 0.0f;
    float panY = 0.0f;
    QPoint lastMousePos;
    CropTransform crop;
    bool rotating = false;
    bool showGrid = false;
    QVector2D aspectScale = {1.0f, 1.0f}; // Aspect ratio correction

    GLuint vao = 0;
    GLuint vbo = 0;
};
