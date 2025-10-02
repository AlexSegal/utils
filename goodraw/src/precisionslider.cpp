#include "precisionslider.h"
#include <QApplication>
#include <QDoubleValidator>
#include <QWheelEvent>
#include <QKeyEvent>
#include <cmath>

PrecisionSlider::PrecisionSlider(QWidget *parent)
    : QWidget(parent)
    , m_minimum(-10.0f)
    , m_maximum(10.0f)
    , m_defaultValue(0.0f)
    , m_currentValue(0.0f)
    , m_singleStep(0.1f)
    , m_pageStep(1.0f)
    , m_decimals(2)
    , m_dragging(false)
    , m_ctrlPressed(false)
{
    // Create layout and components (reduced spacing by ~15%)
    m_layout = new QHBoxLayout(this);
    m_layout->setContentsMargins(0, 0, 0, 0);
    m_layout->setSpacing(4);  // 5 * 0.85 ≈ 4
    
    m_label = new QLabel("Value:", this);
    m_label->setFixedWidth(102);  // 120 * 0.85 = 102
    m_label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    
    // Reduce font size by ~15%
    QFont labelFont = m_label->font();
    labelFont.setPointSizeF(labelFont.pointSizeF() * 0.85);
    m_label->setFont(labelFont);
    
    m_slider = new QSlider(Qt::Horizontal, this);
    m_slider->setRange(0, SLIDER_PRECISION);
    m_slider->setValue(SLIDER_PRECISION / 2); // Start at middle (0.0)
    
    m_lineEdit = new QLineEdit(this);
    m_lineEdit->setMaximumWidth(34);  // 40 * 0.85 = 34
    m_lineEdit->setValidator(new QDoubleValidator(this));
    
    // Reduce font size by ~15%
    QFont lineEditFont = m_lineEdit->font();
    lineEditFont.setPointSizeF(lineEditFont.pointSizeF() * 0.85);
    m_lineEdit->setFont(lineEditFont);
    
    // Layout components with fixed sizes for label and line edit
    m_layout->addWidget(m_label, 0);        // Fixed size label
    m_layout->addWidget(m_slider, 1);       // Stretching slider
    m_layout->addWidget(m_lineEdit, 0);     // Fixed size line edit
    
    // Connect signals
    connect(m_slider, &QSlider::valueChanged, this, &PrecisionSlider::onSliderValueChanged);
    connect(m_lineEdit, &QLineEdit::textChanged, this, &PrecisionSlider::onLineEditChanged);
    connect(m_lineEdit, &QLineEdit::editingFinished, this, &PrecisionSlider::onLineEditFinished);
    
    // Enable mouse tracking for custom drag behavior
    m_slider->setMouseTracking(true);
    m_slider->installEventFilter(this);
    
    // Initialize display
    updateLineEditFromFloat(m_currentValue);
}

void PrecisionSlider::setRange(float minimum, float maximum)
{
    m_minimum = minimum;
    m_maximum = maximum;
    
    // Update validator
    if (auto validator = qobject_cast<QDoubleValidator*>(const_cast<QValidator*>(m_lineEdit->validator()))) {
        validator->setRange(minimum, maximum, m_decimals);
    }
    
    // Clamp current value to new range
    float clampedValue = qBound(minimum, m_currentValue, maximum);
    if (clampedValue != m_currentValue) {
        setValue(clampedValue);
    } else {
        updateSliderFromFloat(m_currentValue);
    }
}

void PrecisionSlider::setDefaultValue(float defaultVal)
{
    m_defaultValue = qBound(m_minimum, defaultVal, m_maximum);
}

void PrecisionSlider::setSingleStep(float step)
{
    m_singleStep = step;
}

void PrecisionSlider::setPageStep(float step)
{
    m_pageStep = step;
}

void PrecisionSlider::setDecimals(int decimals)
{
    m_decimals = decimals;
    
    // Update validator
    if (auto validator = qobject_cast<QDoubleValidator*>(const_cast<QValidator*>(m_lineEdit->validator()))) {
        validator->setDecimals(decimals);
    }
    
    // Update display
    updateLineEditFromFloat(m_currentValue);
}

void PrecisionSlider::setLabel(const QString &text)
{
    m_label->setText(text);
}

float PrecisionSlider::value() const
{
    return m_currentValue;
}

float PrecisionSlider::minimum() const
{
    return m_minimum;
}

float PrecisionSlider::maximum() const
{
    return m_maximum;
}

float PrecisionSlider::defaultValue() const
{
    return m_defaultValue;
}

void PrecisionSlider::setValue(float value)
{
    float clampedValue = qBound(m_minimum, value, m_maximum);
    
    if (qAbs(clampedValue - m_currentValue) > 1e-6) { // Avoid floating point precision issues
        m_currentValue = clampedValue;
        updateSliderFromFloat(clampedValue);
        updateLineEditFromFloat(clampedValue);
        emitValueChanged();
    }
}

void PrecisionSlider::resetToDefault()
{
    setValue(m_defaultValue);
}

void PrecisionSlider::mousePressEvent(QMouseEvent *event)
{
    // Mouse events on the widget itself (not the slider)
    QWidget::mousePressEvent(event);
}

void PrecisionSlider::mouseMoveEvent(QMouseEvent *event)
{
    // Mouse events on the widget itself (not the slider)
    QWidget::mouseMoveEvent(event);
}

void PrecisionSlider::mouseReleaseEvent(QMouseEvent *event)
{
    // Mouse events on the widget itself (not the slider)
    QWidget::mouseReleaseEvent(event);
}

void PrecisionSlider::mouseDoubleClickEvent(QMouseEvent *event)
{
    // Mouse events on the widget itself (not the slider)
    QWidget::mouseDoubleClickEvent(event);
}

void PrecisionSlider::wheelEvent(QWheelEvent *event)
{
    // Mouse wheel for fine adjustments
    float delta = event->angleDelta().y() / 120.0f; // Standard wheel step
    float step = (event->modifiers() & Qt::ControlModifier) ? m_singleStep * 0.1f : m_singleStep;
    
    setValue(m_currentValue + delta * step);
    event->accept();
}

void PrecisionSlider::onSliderValueChanged(int value)
{
    if (!m_dragging) { // Only respond to programmatic changes or user slider interaction
        float floatValue = sliderToFloat(value);
        if (qAbs(floatValue - m_currentValue) > 1e-6) {
            m_currentValue = floatValue;
            updateLineEditFromFloat(floatValue);
            emitValueChanged();
        }
    }
}

void PrecisionSlider::onLineEditChanged()
{
    // Real-time validation and updates as user types
    QString text = m_lineEdit->text();
    bool ok;
    float value = text.toFloat(&ok);
    
    if (ok && value >= m_minimum && value <= m_maximum) {
        m_currentValue = value;
        updateSliderFromFloat(value);
        emitValueChanged();
    }
}

void PrecisionSlider::onLineEditFinished()
{
    // Ensure the final value is properly formatted and within range
    bool ok;
    float value = m_lineEdit->text().toFloat(&ok);
    
    if (ok) {
        setValue(value); // This will clamp and update everything
    } else {
        // Invalid input, revert to current value
        updateLineEditFromFloat(m_currentValue);
    }
}

void PrecisionSlider::updateSliderFromFloat(float value)
{
    int sliderValue = floatToSlider(value);
    
    // Block signals to prevent recursion
    bool wasBlocked = m_slider->blockSignals(true);
    m_slider->setValue(sliderValue);
    m_slider->blockSignals(wasBlocked);
}

void PrecisionSlider::updateLineEditFromFloat(float value)
{
    QString text = QString::number(value, 'f', m_decimals);
    
    // Block signals to prevent recursion
    bool wasBlocked = m_lineEdit->blockSignals(true);
    m_lineEdit->setText(text);
    m_lineEdit->blockSignals(wasBlocked);
}

float PrecisionSlider::sliderToFloat(int sliderValue) const
{
    float ratio = static_cast<float>(sliderValue) / SLIDER_PRECISION;
    return m_minimum + ratio * (m_maximum - m_minimum);
}

int PrecisionSlider::floatToSlider(float value) const
{
    float ratio = (value - m_minimum) / (m_maximum - m_minimum);
    return static_cast<int>(ratio * SLIDER_PRECISION + 0.5f); // Round to nearest
}

bool PrecisionSlider::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == m_slider) {
        switch (event->type()) {
        case QEvent::MouseButtonPress: {
            QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
            if (mouseEvent->button() == Qt::LeftButton) {
                m_dragging = true;
                m_ctrlPressed = (mouseEvent->modifiers() & Qt::ControlModifier);
                m_lastMousePos = mouseEvent->pos();
                m_dragStartValue = m_currentValue;
                emit sliderPressed();
                return true; // Handle the event ourselves
            }
            break;
        }
        case QEvent::MouseMove: {
            if (m_dragging) {
                QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
                int deltaX = mouseEvent->pos().x() - m_lastMousePos.x();
                
                // Calculate sensitivity: normal or 4x reduced with Ctrl
                float sensitivity = m_ctrlPressed ? 0.25f : 1.0f;
                
                // Convert pixel movement to value change
                float range = m_maximum - m_minimum;
                float valueChange = (deltaX * range * sensitivity) / m_slider->width();
                
                float newValue = m_dragStartValue + valueChange;
                setValue(newValue);
                return true; // Handle the event ourselves
            }
            break;
        }
        case QEvent::MouseButtonRelease: {
            QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
            if (mouseEvent->button() == Qt::LeftButton && m_dragging) {
                m_dragging = false;
                emit sliderReleased();
                return true; // Handle the event ourselves
            }
            break;
        }
        case QEvent::MouseButtonDblClick: {
            QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
            if (mouseEvent->button() == Qt::LeftButton) {
                resetToDefault();
                return true; // Handle the event ourselves
            }
            break;
        }
        default:
            break;
        }
    }
    
    return QWidget::eventFilter(obj, event);
}

void PrecisionSlider::emitValueChanged()
{
    emit valueChanged(m_currentValue);
}