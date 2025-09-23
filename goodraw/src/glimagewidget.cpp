#include <QOpenGLShader>
#include <QOpenGLFramebufferObject>
#include <QImage>
#include <QFile>
#include <QOpenGLTexture>
#include <QPainter>
#include <QKeyEvent>
#include <Imath/ImathMatrix.h>
#include <cmath>

#include "glimagewidget.h"
#include "glsl_shaders.h"

GLImageWidget::GLImageWidget(QWidget* parent)
    : QOpenGLWidget(parent)
{
    setFocusPolicy(Qt::StrongFocus); // Enable strong keyboard focus
    setAttribute(Qt::WA_AcceptTouchEvents, false); // Disable touch to avoid conflicts
}

void GLImageWidget::initializeGL() {
    initializeOpenGLFunctions();
    GLenum err = glGetError();
    if (err != GL_NO_ERROR) fprintf(stderr, "OpenGL error after initializeOpenGLFunctions: %d\n", err);

    glEnable(GL_TEXTURE_2D);
    err = glGetError();
    if (err != GL_NO_ERROR) fprintf(stderr, "OpenGL error after glEnable(GL_TEXTURE_2D): %d\n", err);

    // Compile shaders
    program.addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShaderSource);
    err = glGetError();
    if (err != GL_NO_ERROR) fprintf(stderr, "OpenGL error after addShaderFromSourceCode (vertex): %d\n", err);
    program.addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShaderSource);
    err = glGetError();
    if (err != GL_NO_ERROR) fprintf(stderr, "OpenGL error after addShaderFromSourceCode (fragment): %d\n", err);
    program.bindAttributeLocation("aPos", 0);
    program.bindAttributeLocation("aTex", 1);
    program.link();
    err = glGetError();
    if (err != GL_NO_ERROR) fprintf(stderr, "OpenGL error after program.link: %d\n", err);
    int loc_aPos = program.attributeLocation("aPos");
    int loc_aTex = program.attributeLocation("aTex");

    // Setup quad VAO/VBO
    GLfloat vertices[] = {
        0,0, 0,1,
        1,0, 1,1,
        1,1, 1,0,
        0,1, 0,0
    };
    glGenVertexArrays(1, &vao);
    err = glGetError();
    if (err != GL_NO_ERROR) fprintf(stderr, "OpenGL error after glGenVertexArrays: %d\n", err);
    glGenBuffers(1, &vbo);
    err = glGetError();
    if (err != GL_NO_ERROR) fprintf(stderr, "OpenGL error after glGenBuffers: %d\n", err);
    glBindVertexArray(vao);
    err = glGetError();
    if (err != GL_NO_ERROR) fprintf(stderr, "OpenGL error after glBindVertexArray: %d\n", err);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    err = glGetError();
    if (err != GL_NO_ERROR) fprintf(stderr, "OpenGL error after glBindBuffer: %d\n", err);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    err = glGetError();
    if (err != GL_NO_ERROR) fprintf(stderr, "OpenGL error after glBufferData: %d\n", err);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    err = glGetError();
    if (err != GL_NO_ERROR) fprintf(stderr, "OpenGL error after glVertexAttribPointer(0): %d\n", err);
    glEnableVertexAttribArray(0);
    err = glGetError();
    if (err != GL_NO_ERROR) fprintf(stderr, "OpenGL error after glEnableVertexAttribArray(0): %d\n", err);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    err = glGetError();
    if (err != GL_NO_ERROR) fprintf(stderr, "OpenGL error after glVertexAttribPointer(1): %d\n", err);
    glEnableVertexAttribArray(1);
    err = glGetError();
    if (err != GL_NO_ERROR) fprintf(stderr, "OpenGL error after glEnableVertexAttribArray(1): %d\n", err);
    glBindVertexArray(0);
    err = glGetError();
    if (err != GL_NO_ERROR) fprintf(stderr, "OpenGL error after glBindVertexArray(0): %d\n", err);
}

void GLImageWidget::resizeGL(int w, int h) {
    glViewport(0, 0, w, h);
    updateAspectScale(); // Recalculate aspect when widget resizes
}

void GLImageWidget::setImage(const HalfImage &img) {
    fprintf(stderr, "setImage: HERE! Image size: %dx%d\n", img.width, img.height);
    
    imgData = img;

    makeCurrent();
    if(_m_tex) {
        delete _m_tex;
    }

    _m_tex = new QOpenGLTexture(QOpenGLTexture::Target2D);
    _m_tex->setFormat(QOpenGLTexture::RGB32F);
    _m_tex->setSize(img.width, img.height);
    _m_tex->allocateStorage();
    _m_tex->setMinificationFilter(QOpenGLTexture::Nearest);
    _m_tex->setMagnificationFilter(QOpenGLTexture::Nearest);
    _m_tex->setWrapMode(QOpenGLTexture::ClampToEdge);

    // Convert half to float for OpenGL upload
    std::vector<float> buffer(img.width * img.height * 3);
    for (size_t i = 0; i < buffer.size(); ++i) {
        buffer[i] = static_cast<float>(img.data[i]);
    }
    _m_tex->setData(QOpenGLTexture::RGB, QOpenGLTexture::Float32, buffer.data());

    if (!_m_tex) {
        fprintf(stderr, "setImage: No texture set\n");
        return;
    }

    updateAspectScale(); // Calculate aspect ratio for new image
    fitToViewport();     // Auto-fit on load
    update();
}

void GLImageWidget::setExposure(float e){ exposure=e; update(); }
void GLImageWidget::setWB(float r,float g,float b){ wb=QVector3D(r,g,b); update(); }
void GLImageWidget::setContrast(float c){ contrast=c; update(); }
void GLImageWidget::setZoom(float z){ zoom=z; update(); }
void GLImageWidget::setPan(float x,float y){ panX=x; panY=y; update(); }

void GLImageWidget::paintGL() {
    fprintf(stderr, "paintGL: HERE!\n");

    GLenum err = glGetError();
    if (err != GL_NO_ERROR) fprintf(stderr, "OpenGL error at start of paintGL: %d\n", err);

    glViewport(0, 0, width(), height());
    err = glGetError();
    if (err != GL_NO_ERROR) fprintf(stderr, "OpenGL error after glViewport: %d\n", err);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // Black background
    glClear(GL_COLOR_BUFFER_BIT);
    err = glGetError();
    if (err != GL_NO_ERROR) fprintf(stderr, "OpenGL error after glClear: %d\n", err);

    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    err = glGetError();
    if (err != GL_NO_ERROR) fprintf(stderr, "OpenGL error after glDisable: %d\n", err);

    if (!_m_tex) {
        fprintf(stderr, "paintGL BEFORE: No texture set\n");
        return;
    }

    program.bind();
    glActiveTexture(GL_TEXTURE0);
    _m_tex->bind();

    // Calculate transformation matrix using Imath
    // We want to transform from image coordinates to screen coordinates
    Imath::M33f transform;
    transform.makeIdentity();
    
    // Step 1: Center the quad (from 0-1 to -0.5 to 0.5)
    transform = transform * Imath::M33f().setTranslation(Imath::V2f(-0.5f, -0.5f));
    
    // Step 2: Apply aspect ratio correction to fit image in viewport
    transform = transform * Imath::M33f().setScale(Imath::V2f(aspectScale.x(), aspectScale.y()));
    
    // Step 3: Apply zoom (both user zoom and base 2.0 scale)
    float totalZoom = zoom * 2.0f;
    transform = transform * Imath::M33f().setScale(Imath::V2f(totalZoom, totalZoom));
    
    // Step 4: Apply rotation around center
    if (crop.rotation != 0.0f) {
        transform = transform * Imath::M33f().setRotation(crop.rotation);
    }
    
    // Step 5: Apply translation (pan)
    transform = transform * Imath::M33f().setTranslation(Imath::V2f(crop.centerX, crop.centerY));
    
    // Convert to QMatrix3x3 for Qt (note: Imath is row-major, Qt expects column-major)
    QMatrix3x3 qtTransform;
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            qtTransform(j, i) = transform[i][j];  // Transpose for column-major
        }
    }
    
    program.setUniformValue("_m_tex", 0);
    program.setUniformValue("transform", qtTransform);
    program.setUniformValue("exposure", exposure);
    program.setUniformValue("wb", wb);
    program.setUniformValue("contrast", contrast);
    program.setUniformValue("showGrid", showGrid);

    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    glBindVertexArray(0);
    err = glGetError();
    if (err != GL_NO_ERROR) fprintf(stderr, "OpenGL error after draw: %d\n", err);

    _m_tex->release();
    program.release();

    if (!_m_tex) {
        fprintf(stderr, "paintGL AFTER: No texture set\n");
        return;
    }

}

void GLImageWidget::mousePressEvent(QMouseEvent *event){
    setFocus(); // Ensure widget has focus for keyboard events
    lastMousePos=event->pos();
    if(event->button()==Qt::RightButton) rotating=true;
    else rotating=false;
    showGrid=rotating;
}

void GLImageWidget::mouseMoveEvent(QMouseEvent *event){
    QPointF delta=event->pos()-lastMousePos;
    if(rotating){
        crop.rotation += delta.x()*0.01f;
    } else if(event->buttons() & Qt::LeftButton){
        crop.centerX += delta.x()/width()/zoom;
        crop.centerY -= delta.y()/height()/zoom;
    }
    lastMousePos=event->pos();
    update();
}

void GLImageWidget::wheelEvent(QWheelEvent *event){
    // Get mouse position in widget coordinates
    QPoint mousePos = event->position().toPoint();
    
    // Convert to normalized coordinates (-1 to 1) to match clip space
    float clipX = (2.0f * mousePos.x() / width()) - 1.0f;
    float clipY = 1.0f - (2.0f * mousePos.y() / height());
    
    // Build the current transformation matrix (same as in paintGL)
    Imath::M33f transform;
    transform.makeIdentity();
    transform = transform * Imath::M33f().setTranslation(Imath::V2f(-0.5f, -0.5f));
    transform = transform * Imath::M33f().setScale(Imath::V2f(aspectScale.x(), aspectScale.y()));
    
    float oldTotalZoom = zoom * 2.0f;
    transform = transform * Imath::M33f().setScale(Imath::V2f(oldTotalZoom, oldTotalZoom));
    
    if (crop.rotation != 0.0f) {
        transform = transform * Imath::M33f().setRotation(crop.rotation);
    }
    transform = transform * Imath::M33f().setTranslation(Imath::V2f(crop.centerX, crop.centerY));
    
    // Invert the matrix to get from clip space to image space
    Imath::M33f invTransform = transform.inverse();
    
    // Transform mouse clip coordinates to image space
    Imath::V3f clipPoint(clipX, clipY, 1.0f);
    Imath::V3f imagePoint = clipPoint * invTransform;
    
    // Apply zoom change
    float delta = event->angleDelta().y() / 120.0f;
    zoom *= pow(1.1f, delta);
    
    // Build new transformation matrix with new zoom
    Imath::M33f newTransform;
    newTransform.makeIdentity();
    newTransform = newTransform * Imath::M33f().setTranslation(Imath::V2f(-0.5f, -0.5f));
    newTransform = newTransform * Imath::M33f().setScale(Imath::V2f(aspectScale.x(), aspectScale.y()));
    
    float newTotalZoom = zoom * 2.0f;
    newTransform = newTransform * Imath::M33f().setScale(Imath::V2f(newTotalZoom, newTotalZoom));
    
    if (crop.rotation != 0.0f) {
        newTransform = newTransform * Imath::M33f().setRotation(crop.rotation);
    }
    
    // We want: newTransform * imagePoint = clipPoint
    // So: translation = clipPoint - (partialTransform * imagePoint)
    Imath::M33f partialTransform;
    partialTransform.makeIdentity();
    partialTransform = partialTransform * Imath::M33f().setTranslation(Imath::V2f(-0.5f, -0.5f));
    partialTransform = partialTransform * Imath::M33f().setScale(Imath::V2f(aspectScale.x(), aspectScale.y()));
    partialTransform = partialTransform * Imath::M33f().setScale(Imath::V2f(newTotalZoom, newTotalZoom));
    if (crop.rotation != 0.0f) {
        partialTransform = partialTransform * Imath::M33f().setRotation(crop.rotation);
    }
    
    Imath::V3f transformedPoint = imagePoint * partialTransform;
    Imath::V2f requiredTranslation = Imath::V2f(clipPoint.x - transformedPoint.x, clipPoint.y - transformedPoint.y);
    
    crop.centerX = requiredTranslation.x;
    crop.centerY = requiredTranslation.y;
    
    fprintf(stderr, "Mouse: (%d,%d) -> clip(%f,%f) -> image(%f,%f) -> newPan(%f,%f)\n",
            mousePos.x(), mousePos.y(), clipX, clipY, imagePoint.x, imagePoint.y, crop.centerX, crop.centerY);
    
    update();
}

void GLImageWidget::keyPressEvent(QKeyEvent *event) {
    fprintf(stderr, "Key pressed: %d (F key is %d)\n", event->key(), Qt::Key_F);
    if (event->key() == Qt::Key_F) {
        fprintf(stderr, "F key detected, calling fitToViewport\n");
        fitToViewport();
    } else {
        QOpenGLWidget::keyPressEvent(event);
    }
}

void GLImageWidget::fitToViewport() {
    fprintf(stderr, "fitToViewport called: zoom=%f -> 1.0, pan=(%f,%f) -> (0,0), rotation=%f -> 0\n", 
            zoom, crop.centerX, crop.centerY, crop.rotation);
    zoom = 1.0f;  // Changed from 2.0f to 1.0f for better initial fit
    crop.centerX = 0.0f;
    crop.centerY = 0.0f;
    crop.rotation = 0.0f;
    update();
}

bool GLImageWidget::exportImage(const QString &filename){
    if(!_m_tex) return false;

    makeCurrent();
    QOpenGLFramebufferObject fbo(_m_tex->width(),_m_tex->height());
    fbo.bind();

    paintGL(); // render to FBO

    QImage img=fbo.toImage();
    fbo.release();

    return img.save(filename);
}

void GLImageWidget::updateAspectScale() {
    if (!_m_tex || imgData.width == 0 || imgData.height == 0) {
        aspectScale = QVector2D(1.0f, 1.0f);
        fprintf(stderr, "updateAspectScale: No texture or zero size, aspectScale = (1,1)\n");
        return;
    }

    float widgetAspect = static_cast<float>(width()) / static_cast<float>(height());
    float imageAspect = static_cast<float>(imgData.width) / static_cast<float>(imgData.height);

    fprintf(stderr, "updateAspectScale: widget=%dx%d (aspect=%.3f), image=%dx%d (aspect=%.3f)\n", 
            width(), height(), widgetAspect, imgData.width, imgData.height, imageAspect);

    if (imageAspect > widgetAspect) {
        // Image is wider than widget - fit to width, scale down height
        aspectScale = QVector2D(1.0f, widgetAspect / imageAspect);
        fprintf(stderr, "updateAspectScale: Image wider, aspectScale = (%.3f, %.3f)\n", 
                aspectScale.x(), aspectScale.y());
    } else {
        // Image is taller than widget - fit to height, scale down width  
        aspectScale = QVector2D(imageAspect / widgetAspect, 1.0f);
        fprintf(stderr, "updateAspectScale: Image taller, aspectScale = (%.3f, %.3f)\n", 
                aspectScale.x(), aspectScale.y());
    }
}
