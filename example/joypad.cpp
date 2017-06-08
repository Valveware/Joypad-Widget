#include "joypad.h"

#include <QPainter>
#include <QParallelAnimationGroup>
#include <QPropertyAnimation>
#include <QMouseEvent>
#include <math.h>


template<typename T>
T constrain(T Value, T Min, T Max)
{
  return (Value < Min)? Min : (Value > Max)? Max : Value;
}


JoyPad::JoyPad(QWidget *parent) : QWidget(parent),
    m_returnAnimation(new QParallelAnimationGroup(this)),
    m_x(0), m_y(0)
{
    QPropertyAnimation *xAnimation = new QPropertyAnimation(this, "x");
    xAnimation->setEndValue(0.f);
    xAnimation->setDuration(400);
    xAnimation->setEasingCurve(QEasingCurve::OutSine);

    QPropertyAnimation *yAnimation = new QPropertyAnimation(this, "y");
    yAnimation->setEndValue(0.f);
    yAnimation->setDuration(400);
    yAnimation->setEasingCurve(QEasingCurve::OutSine);

    m_returnAnimation->addAnimation(xAnimation);
    m_returnAnimation->addAnimation(yAnimation);
}

/**
 * @brief JoyPad::x
 * @return
 */
float JoyPad::x() const
{
    return m_x;
}

/**
 * @brief JoyPad::y
 * @return
 */
float JoyPad::y() const
{
    return m_y;
}

/**
 * @brief JoyPad::setX
 * @param value of x axis from -1 to 1
 */
void JoyPad::setX(float value)
{
    m_x = constrain(value, -1.f, 1.f);

    qreal radius = ( m_bounds.width() - m_knopBounds.width() ) / 2;
    m_knopBounds.moveCenter(QPointF(m_bounds.center().x()+ m_x * radius, m_knopBounds.y()));

    update();
    emit xChanged(m_x);
}

/**
 * @brief JoyPad::setY
 * @param value of y axis from -1 to 1
 */
void JoyPad::setY(float value)
{
    m_y = constrain(value, -1.f, 1.f);

    qreal radius = ( m_bounds.width() - m_knopBounds.width() ) / 2;
    m_knopBounds.moveCenter(QPointF(m_knopBounds.center().x(), m_bounds.center().y() + m_y * radius));

    update();
    emit yChanged(m_y);
}

/**
 * @brief JoyPad::resizeEvent
 * @param event
 *
 * calculates a square bounding rect for the background and the knob
 */
void JoyPad::resizeEvent(QResizeEvent *event)
{
    Q_UNUSED(event)

    float a = qMin(width(), height());

    // 2% margin for the gods
    a -= a*0.02;

    // old dimensions needed to scale the knob
    float oldA = m_bounds.width();
    QPointF fromCenterToKnop = m_knopBounds.center() - m_bounds.center();

    m_bounds = QRect((width() -a)/2, (height()-a)/2, a, a);
    m_knopBounds.setWidth(a * 0.3);
    m_knopBounds.setHeight(a*0.3);

    // in case of first resize event move knob to center
    if (oldA == 0)
    {
        m_knopBounds.moveCenter(m_bounds.center());
        return;
    }

    fromCenterToKnop *= a / oldA;
    m_knopBounds.moveCenter(m_bounds.center() + fromCenterToKnop);
}

/**
 * @brief JoyPad::paintEvent
 * @param event
 */
void JoyPad::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::HighQualityAntialiasing);

    // draw background
    QRadialGradient gradient(m_bounds.center(), m_bounds.width()/2, m_bounds.center());
    gradient.setFocalRadius(m_bounds.width()*0.3);
    gradient.setCenterRadius(m_bounds.width()*0.7);
    gradient.setColorAt(0, Qt::white);
    gradient.setColorAt(1, Qt::lightGray);

    painter.setPen(QPen(QBrush(Qt::gray), m_bounds.width()* 0.005));
    painter.setBrush(QBrush(gradient));
    painter.drawEllipse(m_bounds);

    // draw crosshair
    painter.setPen(QPen(QBrush(Qt::gray), m_bounds.width()* 0.005));
    painter.drawLine(QPointF(m_bounds.left(), m_bounds.center().y()), QPointF(m_bounds.center().x() - m_bounds.width()*0.35, m_bounds.center().y()));
    painter.drawLine(QPointF(m_bounds.center().x() + m_bounds.width()*0.35, m_bounds.center().y()), QPointF(m_bounds.right(), m_bounds.center().y()));
    painter.drawLine(QPointF(m_bounds.center().x(), m_bounds.top()), QPointF(m_bounds.center().x(), m_bounds.center().y() - m_bounds.width()*0.35));
    painter.drawLine(QPointF(m_bounds.center().x(), m_bounds.center().y() + m_bounds.width()*0.35), QPointF(m_bounds.center().x(), m_bounds.bottom()));

    // draw knob
    gradient = QRadialGradient(m_knopBounds.center(), m_knopBounds.width()/2, m_knopBounds.center());
    gradient.setColorAt(0, Qt::gray);
    gradient.setColorAt(1, Qt::darkGray);
    gradient.setFocalRadius(m_knopBounds.width()*0.2);
    gradient.setCenterRadius(m_knopBounds.width()*0.5);

    painter.setPen(QPen(QBrush(Qt::darkGray), m_bounds.width()*0.005));
    painter.setBrush(QBrush(gradient));
    painter.drawEllipse(m_knopBounds);
}

/**
 * @brief JoyPad::mousePressEvent
 * @param event
 */
void JoyPad::mousePressEvent(QMouseEvent *event)
{
    if (m_knopBounds.contains(event->pos()))
    {
        m_returnAnimation->stop();
        m_lastPos = event->pos();
        knopPressed = true;
    }    
}

/**
 * @brief JoyPad::mouseReleaseEvent
 * @param event
 */
void JoyPad::mouseReleaseEvent(QMouseEvent *event)
{
    Q_UNUSED(event)

    knopPressed = false;
    m_returnAnimation->start();
}

/**
 * @brief JoyPad::mouseMoveEvent
 * @param event
 */
void JoyPad::mouseMoveEvent(QMouseEvent *event)
{
    if (!knopPressed) return;

    // moved distance
    QPointF dPos = event->pos() - m_lastPos; 

    // change the distance sligthly to guarantee overlaping knop and pointer
    dPos += 0.05 * (event->pos() - m_knopBounds.center());

    QPointF fromCenterToKnop = m_knopBounds.center() + dPos - m_bounds.center();

    qreal radius = ( m_bounds.width() - m_knopBounds.width() ) / 2;

    fromCenterToKnop.setX(constrain(fromCenterToKnop.x(), -radius, radius));
    fromCenterToKnop.setY(constrain(fromCenterToKnop.y(), -radius, radius));

    m_knopBounds.moveCenter(fromCenterToKnop + m_bounds.center());
    m_lastPos = event->pos();

    update();

    if (radius == 0) return;
    float x = ( m_knopBounds.center().x() - m_bounds.center().x() ) / radius;
    float y = ( m_knopBounds.center().y() - m_bounds.center().y() ) / radius;

    if (m_x !=x)
    {
        m_x = x;
        emit xChanged(m_x);
    }

    if (m_y !=y)
    {
        m_y = y;
        emit yChanged(m_y);
    }
}


