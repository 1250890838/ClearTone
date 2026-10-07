#include "windowbutton.h"

#include <QHoverEvent>
#include <QMouseEvent>

WindowButton::WindowButton(QQuickItem *parent)
    : QQuickItem(parent)
{
    setAcceptHoverEvents(true);
    setAcceptedMouseButtons(Qt::LeftButton);
}

WindowButton::Role WindowButton::role() const
{
    return m_role;
}

void WindowButton::setRole(Role role)
{
    if (m_role == role)
        return;
    m_role = role;
    emit roleChanged();
}

bool WindowButton::isHovered() const
{
    return m_hovered;
}

bool WindowButton::isPressed() const
{
    return m_pressed;
}

void WindowButton::setHovered(bool hovered)
{
    if (m_hovered == hovered)
        return;
    m_hovered = hovered;
    emit hoveredChanged();
}

void WindowButton::setPressed(bool pressed)
{
    if (m_pressed == pressed)
        return;
    m_pressed = pressed;
    emit pressedChanged();
}

void WindowButton::emitClicked()
{
    emit clicked();
}

void WindowButton::hoverEnterEvent(QHoverEvent *event)
{
    setHovered(true);
    QQuickItem::hoverEnterEvent(event);
}

void WindowButton::hoverLeaveEvent(QHoverEvent *event)
{
    setHovered(false);
    QQuickItem::hoverLeaveEvent(event);
}

void WindowButton::mousePressEvent(QMouseEvent *event)
{
    event->accept();
    setPressed(true);
}

void WindowButton::mouseReleaseEvent(QMouseEvent *event)
{
    event->accept();
    if (!m_pressed)
        return;
    setPressed(false);
    emit clicked();
}

void WindowButton::mouseUngrabEvent()
{
    setPressed(false);
}
