#ifndef FRAMELESSWINDOW_H
#define FRAMELESSWINDOW_H

#include <QPointer>
#include <QRectF>
#include <QQuickWindow>
#include <QVector>
#include <QtQml/qqmlregistration.h>

class FramelessWindowAttached;
class WindowButton;

class FramelessWindow : public QQuickWindow
{
    Q_OBJECT
    QML_ELEMENT
    QML_ATTACHED(FramelessWindowAttached)

    Q_PROPERTY(qreal resizeBorderWidth READ resizeBorderWidth WRITE setResizeBorderWidth
                   NOTIFY resizeBorderWidthChanged)

    Q_PROPERTY(bool resizeEnabled READ isResizeEnabled WRITE setResizeEnabled
                   NOTIFY resizeEnabledChanged)

    Q_PROPERTY(bool snapLayoutsEnabled READ isSnapLayoutsEnabled WRITE setSnapLayoutsEnabled
                   NOTIFY snapLayoutsEnabledChanged)

    Q_PROPERTY(bool systemMenuEnabled READ isSystemMenuEnabled WRITE setSystemMenuEnabled
                   NOTIFY systemMenuEnabledChanged)

    Q_PROPERTY(bool darkMode READ isDarkMode WRITE setDarkMode NOTIFY darkModeChanged)

    Q_PROPERTY(bool maximized READ isMaximized NOTIFY maximizedChanged)

public:
    explicit FramelessWindow(QWindow *parent = nullptr);
    ~FramelessWindow() override;

    static FramelessWindowAttached *qmlAttachedProperties(QObject *object);

    qreal resizeBorderWidth() const;
    void setResizeBorderWidth(qreal width);

    bool isResizeEnabled() const;
    void setResizeEnabled(bool enabled);

    bool isSnapLayoutsEnabled() const;
    void setSnapLayoutsEnabled(bool enabled);

    bool isSystemMenuEnabled() const;
    void setSystemMenuEnabled(bool enabled);

    bool isDarkMode() const;
    void setDarkMode(bool dark);

    bool isMaximized() const;

    Q_INVOKABLE void toggleMaximize();

    Q_INVOKABLE void refreshNativeChrome();

signals:
    void resizeBorderWidthChanged();
    void resizeEnabledChanged();
    void snapLayoutsEnabledChanged();
    void systemMenuEnabledChanged();
    void darkModeChanged();
    void maximizedChanged();

protected:
    bool event(QEvent *event) override;
    bool nativeEvent(const QByteArray &eventType, void *message, qintptr *result) override;

private:
    struct HitRegion
    {
        QRectF rect;
        int hitCode;
    };

    void applyNativeChrome();
    void applyDwmAttributes();
    void rebuildRegions();
    int hitTest(const QPoint &screenPos);
    WindowButton *buttonAt(const QPoint &screenPos, int *hitCodeOut = nullptr);
    void updateHoverState(const QPoint &screenPos);
    void clearHoverState();
    void setPressedButton(WindowButton *button);
    qreal effectiveResizeBorderWidth();

    void syncMaximizedState();

    qreal m_resizeBorderWidth = -1;
    bool m_resizeEnabled = true;
    bool m_snapLayoutsEnabled = true;
    bool m_systemMenuEnabled = true;
    bool m_darkMode = true;
    bool m_chromeApplied = false;
    bool m_maximized = false;

    QVector<HitRegion> m_buttonRegions;
    QVector<QRectF> m_clientRegions;
    QVector<QRectF> m_dragRegions;

    QPointer<WindowButton> m_pressedButton;
    QPoint m_lastMousePos;

    void *m_hwnd = nullptr;
};

class FramelessWindowAttached : public QObject
{
    Q_OBJECT
    QML_ANONYMOUS
    Q_PROPERTY(bool dragRegion READ isDragRegion WRITE setDragRegion NOTIFY dragRegionChanged)
    Q_PROPERTY(bool clientArea READ isClientArea WRITE setClientArea NOTIFY clientAreaChanged)

public:
    explicit FramelessWindowAttached(QObject *parent = nullptr);

    bool isDragRegion() const;
    void setDragRegion(bool on);

    bool isClientArea() const;
    void setClientArea(bool on);

signals:
    void dragRegionChanged();
    void clientAreaChanged();

private:
    bool m_dragRegion = false;
    bool m_clientArea = false;
};

#endif // FRAMELESSWINDOW_H
