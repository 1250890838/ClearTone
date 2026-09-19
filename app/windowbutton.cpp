#include "windowbutton.h"

WindowButton::WindowButton(QQuickItem *parent)
    : QQuickItem(parent)
{
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
