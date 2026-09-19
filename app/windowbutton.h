#ifndef WINDOWBUTTON_H
#define WINDOWBUTTON_H

#include <QQuickItem>
#include <QtQml/qqmlregistration.h>

class WindowButton : public QQuickItem
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(Role role READ role WRITE setRole NOTIFY roleChanged)
    Q_PROPERTY(bool hovered READ isHovered NOTIFY hoveredChanged)
    Q_PROPERTY(bool pressed READ isPressed NOTIFY pressedChanged)

public:
    enum Role {
        Minimize = 0,
        Maximize,
        Close,
    };
    Q_ENUM(Role)

    explicit WindowButton(QQuickItem *parent = nullptr);

    Role role() const;
    void setRole(Role role);

    bool isHovered() const;
    bool isPressed() const;

    void setHovered(bool hovered);
    void setPressed(bool pressed);
    void emitClicked();

signals:
    void roleChanged();
    void hoveredChanged();
    void pressedChanged();

    void clicked();

private:
    Role m_role = Minimize;
    bool m_hovered = false;
    bool m_pressed = false;
};

#endif // WINDOWBUTTON_H
