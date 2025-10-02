#ifndef PRECISIONSLIDER_H
#define PRECISIONSLIDER_H

#include <QWidget>
#include <QSlider>
#include <QLineEdit>
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QKeyEvent>

/**
 * @brief Professional precision slider widget for RAW image adjustments
 * 
 * Features:
 * - Float precision values instead of integer
 * - Integrated text field for direct value entry
 * - Ctrl+drag for 4x reduced sensitivity (fine adjustment)
 * - Double-click to reset to default value
 * - Configurable range and default value
 * - Real-time value updates during dragging
 */
class PrecisionSlider : public QWidget
{
    Q_OBJECT

public:
    explicit PrecisionSlider(QWidget *parent = nullptr);
    
    // Configuration
    void setRange(float minimum, float maximum);
    void setDefaultValue(float defaultVal);
    void setSingleStep(float step);
    void setPageStep(float step);
    void setDecimals(int decimals);
    void setLabel(const QString &text);
    
    // Value access
    float value() const;
    float minimum() const;
    float maximum() const;
    float defaultValue() const;
    
public slots:
    void setValue(float value);
    void resetToDefault();
    
signals:
    void valueChanged(float value);
    void sliderPressed();
    void sliderReleased();
    
protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;
    
private slots:
    void onSliderValueChanged(int value);
    void onLineEditChanged();
    void onLineEditFinished();
    
private:
    void updateSliderFromFloat(float value);
    void updateLineEditFromFloat(float value);
    float sliderToFloat(int sliderValue) const;
    int floatToSlider(float value) const;
    void emitValueChanged();
    
    // UI components
    QHBoxLayout *m_layout;
    QLabel *m_label;
    QSlider *m_slider;
    QLineEdit *m_lineEdit;
    
    // Value management
    float m_minimum;
    float m_maximum;
    float m_defaultValue;
    float m_currentValue;
    float m_singleStep;
    float m_pageStep;
    int m_decimals;
    
    // Interaction state
    bool m_dragging;
    bool m_ctrlPressed;
    QPoint m_lastMousePos;
    float m_dragStartValue;
    
    // Slider precision (internal slider uses this many steps)
    static const int SLIDER_PRECISION = 10000;
};

#endif // PRECISIONSLIDER_H