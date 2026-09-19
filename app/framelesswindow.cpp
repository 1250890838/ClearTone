#include "framelesswindow.h"
#include "windowbutton.h"

#include <QEvent>
#include <QQuickItem>
#include <QTimer>
#include <QtQml/qqml.h>

#include <utility>

#ifdef Q_OS_WIN
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  ifndef NOMINMAX
#    define NOMINMAX
#  endif
#  include <windows.h>
#  include <windowsx.h>
#  include <dwmapi.h>
#endif

#ifdef Q_OS_WIN

namespace {

constexpr DWORD kDwmwaUseImmersiveDarkModeBefore20H1 = 19;

constexpr UINT kWMNCUAHDrawCaption = 0x00AE;
constexpr UINT kWMNCUAHDrawFrame = 0x00AF;

bool isCaptionHitCode(int code)
{
    return code == HTMINBUTTON || code == HTMAXBUTTON || code == HTCLOSE;
}

int hitCodeForRole(WindowButton::Role role)
{
    switch (role) {
    case WindowButton::Minimize:
        return HTMINBUTTON;
    case WindowButton::Maximize:
        return HTMAXBUTTON;
    case WindowButton::Close:
        return HTCLOSE;
    }
    return HTCLIENT;
}

QObject *attachedOf(const QQuickItem *item)
{
    return qmlAttachedPropertiesObject<FramelessWindow>(item, false);
}

bool flagOf(const QQuickItem *item, const char *name)
{
    const QObject *attached = attachedOf(item);
    return attached && attached->property(name).toBool();
}

QRectF sceneRectOf(const QQuickItem *item)
{
    return QRectF(item->mapToScene(QPointF(0, 0)), item->size());
}

} // namespace
#endif // Q_OS_WIN

FramelessWindow::FramelessWindow(QWindow *parent)
    : QQuickWindow(parent)
{
    setFlags(Qt::Window | Qt::FramelessWindowHint | Qt::WindowSystemMenuHint
             | Qt::WindowMinimizeButtonHint | Qt::WindowMaximizeButtonHint
             | Qt::WindowCloseButtonHint);

#ifdef Q_OS_WIN
    connect(this, &QWindow::screenChanged, this, [this] { applyDwmAttributes(); });
#endif
}

FramelessWindow::~FramelessWindow() = default;

FramelessWindowAttached *FramelessWindow::qmlAttachedProperties(QObject *object)
{
    return new FramelessWindowAttached(object);
}

qreal FramelessWindow::resizeBorderWidth() const
{
    return m_resizeBorderWidth;
}

void FramelessWindow::setResizeBorderWidth(qreal width)
{
    if (qFuzzyCompare(m_resizeBorderWidth, width))
        return;
    m_resizeBorderWidth = width;
    emit resizeBorderWidthChanged();
}

bool FramelessWindow::isResizeEnabled() const
{
    return m_resizeEnabled;
}

void FramelessWindow::setResizeEnabled(bool enabled)
{
    if (m_resizeEnabled == enabled)
        return;
    m_resizeEnabled = enabled;
    emit resizeEnabledChanged();
}

bool FramelessWindow::isSnapLayoutsEnabled() const
{
    return m_snapLayoutsEnabled;
}

void FramelessWindow::setSnapLayoutsEnabled(bool enabled)
{
    if (m_snapLayoutsEnabled == enabled)
        return;
    m_snapLayoutsEnabled = enabled;
    emit snapLayoutsEnabledChanged();
}

bool FramelessWindow::isSystemMenuEnabled() const
{
    return m_systemMenuEnabled;
}

void FramelessWindow::setSystemMenuEnabled(bool enabled)
{
    if (m_systemMenuEnabled == enabled)
        return;
    m_systemMenuEnabled = enabled;
    emit systemMenuEnabledChanged();
}

bool FramelessWindow::isDarkMode() const
{
    return m_darkMode;
}

void FramelessWindow::setDarkMode(bool dark)
{
    if (m_darkMode == dark)
        return;
    m_darkMode = dark;
    emit darkModeChanged();
#ifdef Q_OS_WIN
    applyDwmAttributes();
#endif
}

bool FramelessWindow::isMaximized() const
{
    return m_maximized;
}

void FramelessWindow::syncMaximizedState()
{
#ifdef Q_OS_WIN
    const HWND hwnd = static_cast<HWND>(m_hwnd);
    const bool zoomed = hwnd && IsZoomed(hwnd);
#else
    const bool zoomed = visibility() == QWindow::Maximized;
#endif
    if (m_maximized == zoomed)
        return;
    m_maximized = zoomed;
    emit maximizedChanged();
}

void FramelessWindow::toggleMaximize()
{
    syncMaximizedState();
    if (m_maximized)
        showNormal();
    else
        showMaximized();
}

void FramelessWindow::refreshNativeChrome()
{
#ifdef Q_OS_WIN
    applyNativeChrome();
#endif
}

bool FramelessWindow::event(QEvent *event)
{
    const bool result = QQuickWindow::event(event);
#ifdef Q_OS_WIN
    if (event->type() == QEvent::Show && !m_chromeApplied)
        applyNativeChrome();
#endif
    return result;
}

#ifdef Q_OS_WIN

void FramelessWindow::applyNativeChrome()
{
    const HWND hwnd = reinterpret_cast<HWND>(winId());
    if (!hwnd) {
        QTimer::singleShot(0, this, &FramelessWindow::applyNativeChrome);
        return;
    }

    if (!m_chromeApplied) {
        m_chromeApplied = true;
        m_hwnd = hwnd;

        LONG_PTR style = GetWindowLongPtrW(hwnd, GWL_STYLE);
        style &= ~(WS_POPUP | WS_CHILD);
        style |= WS_THICKFRAME | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX;
        SetWindowLongPtrW(hwnd, GWL_STYLE, style);

        LONG_PTR exStyle = GetWindowLongPtrW(hwnd, GWL_EXSTYLE);
        exStyle &= ~(WS_EX_CLIENTEDGE | WS_EX_STATICEDGE | WS_EX_DLGMODALFRAME);
        SetWindowLongPtrW(hwnd, GWL_EXSTYLE, exStyle);

        SetWindowPos(hwnd, nullptr, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
    }

    applyDwmAttributes();
    syncMaximizedState();
}

void FramelessWindow::applyDwmAttributes()
{
    const HWND hwnd = static_cast<HWND>(m_hwnd);
    if (!hwnd)
        return;

    const BOOL dark = m_darkMode ? TRUE : FALSE;
    if (FAILED(DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &dark, sizeof(dark)))) {
        DwmSetWindowAttribute(hwnd, kDwmwaUseImmersiveDarkModeBefore20H1, &dark, sizeof(dark));
    }

    const int corner = IsZoomed(hwnd) ? DWMWCP_DONOTROUND : DWMWCP_ROUND;
    DwmSetWindowAttribute(hwnd, DWMWA_WINDOW_CORNER_PREFERENCE, &corner, sizeof(corner));
}

void FramelessWindow::rebuildRegions()
{
    m_buttonRegions.clear();
    m_clientRegions.clear();
    m_dragRegions.clear();

    QQuickItem *root = contentItem();
    if (!root)
        return;

    QList<QQuickItem *> stack{root};
    while (!stack.isEmpty()) {
        QQuickItem *item = stack.takeLast();

        if (!item->isVisible() || !item->isEnabled())
            continue;

        if (auto *button = qobject_cast<WindowButton *>(item)) {
            m_buttonRegions.append({sceneRectOf(item), hitCodeForRole(button->role())});
        } else if (flagOf(item, "clientArea")) {
            m_clientRegions.append(sceneRectOf(item));
        } else if (flagOf(item, "dragRegion")) {
            m_dragRegions.append(sceneRectOf(item));
        }

        const QList<QQuickItem *> children = item->childItems();
        for (QQuickItem *child : children)
            stack.append(child);
    }
}

qreal FramelessWindow::effectiveResizeBorderWidth()
{
    if (m_resizeBorderWidth >= 0)
        return m_resizeBorderWidth;

    const HWND hwnd = static_cast<HWND>(m_hwnd);
    const UINT dpi = hwnd ? GetDpiForWindow(hwnd) : 96;
    const int frame = GetSystemMetricsForDpi(SM_CXSIZEFRAME, dpi);
    const int padded = GetSystemMetricsForDpi(SM_CXPADDEDBORDER, dpi);
    return (frame + padded) / devicePixelRatio();
}

static QPointF toLocalDip(QWindow *window, HWND hwnd, const QPoint &screenPos)
{
    const qreal dpr = window->devicePixelRatio();
    if (!hwnd || dpr <= 0)
        return QPointF(-1, -1);

    POINT origin{0, 0};
    ClientToScreen(hwnd, &origin);

    return QPointF((screenPos.x() - origin.x) / dpr, (screenPos.y() - origin.y) / dpr);
}

int FramelessWindow::hitTest(const QPoint &screenPos)
{
    const HWND hwnd = static_cast<HWND>(m_hwnd);
    if (!hwnd || !isVisible())
        return HTCLIENT;

    rebuildRegions();

    const QPointF pos = toLocalDip(this, hwnd, screenPos);

    RECT client{};
    GetClientRect(hwnd, &client);
    const qreal dpr = devicePixelRatio();
    const QSizeF size((client.right - client.left) / dpr, (client.bottom - client.top) / dpr);

    const bool zoomed = IsZoomed(hwnd) || (windowStates() & Qt::WindowFullScreen);

    if (!zoomed && m_resizeEnabled) {
        const qreal b = effectiveResizeBorderWidth();
        if (b > 0) {
            const bool left = pos.x() < b;
            const bool right = pos.x() >= size.width() - b;
            const bool top = pos.y() < b;
            const bool bottom = pos.y() >= size.height() - b;
            if (top || left || right || bottom) {
                if (left && top)
                    return HTTOPLEFT;
                if (right && top)
                    return HTTOPRIGHT;
                if (left && bottom)
                    return HTBOTTOMLEFT;
                if (right && bottom)
                    return HTBOTTOMRIGHT;
                if (left)
                    return HTLEFT;
                if (right)
                    return HTRIGHT;
                if (top)
                    return HTTOP;
                return HTBOTTOM;
            }
        }
    }

    for (const HitRegion &region : std::as_const(m_buttonRegions)) {
        if (region.rect.contains(pos))
            return region.hitCode;
    }

    for (const QRectF &region : std::as_const(m_clientRegions)) {
        if (region.contains(pos))
            return HTCLIENT;
    }

    for (const QRectF &region : std::as_const(m_dragRegions)) {
        if (region.contains(pos))
            return HTCAPTION;
    }

    return HTCLIENT;
}

WindowButton *FramelessWindow::buttonAt(const QPoint &screenPos, int *hitCodeOut)
{
    const HWND hwnd = static_cast<HWND>(m_hwnd);
    if (!hwnd)
        return nullptr;

    const QPointF pos = toLocalDip(this, hwnd, screenPos);

    WindowButton *found = nullptr;
    const QList<WindowButton *> buttons = findChildren<WindowButton *>();
    for (WindowButton *button : buttons) {
        if (!button->isVisible() || !button->isEnabled())
            continue;
        if (sceneRectOf(button).contains(pos))
            found = button;
    }

    if (found && hitCodeOut)
        *hitCodeOut = hitCodeForRole(found->role());
    return found;
}

void FramelessWindow::updateHoverState(const QPoint &screenPos)
{
    m_lastMousePos = screenPos;
    const HWND hwnd = static_cast<HWND>(m_hwnd);
    if (!hwnd)
        return;

    const QPointF pos = toLocalDip(this, hwnd, screenPos);

    const QList<WindowButton *> buttons = findChildren<WindowButton *>();
    for (WindowButton *button : buttons) {
        const bool inside = button->isVisible() && button->isEnabled()
                            && sceneRectOf(button).contains(pos);
        button->setHovered(inside);
        if (button == m_pressedButton && button->isPressed() != inside)
            button->setPressed(inside);
    }

    if (m_pressedButton && !m_pressedButton->isHovered())
        setPressedButton(nullptr);
}

void FramelessWindow::clearHoverState()
{
    const QList<WindowButton *> buttons = findChildren<WindowButton *>();
    for (WindowButton *button : buttons)
        button->setHovered(false);
    setPressedButton(nullptr);
}

void FramelessWindow::setPressedButton(WindowButton *button)
{
    if (m_pressedButton == button)
        return;
    if (m_pressedButton)
        m_pressedButton->setPressed(false);
    m_pressedButton = button;
    if (m_pressedButton)
        m_pressedButton->setPressed(true);
}

bool FramelessWindow::nativeEvent(const QByteArray &eventType, void *message, qintptr *result)
{
#ifdef Q_OS_WIN
    if (eventType != "windows_generic_MSG")
        return QQuickWindow::nativeEvent(eventType, message, result);

    MSG *msg = static_cast<MSG *>(message);
    const HWND hwnd = msg->hwnd;

    if (!m_hwnd || hwnd != static_cast<HWND>(m_hwnd))
        return QQuickWindow::nativeEvent(eventType, message, result);

    switch (msg->message) {
    case WM_NCCALCSIZE:
        if (msg->wParam == TRUE) {
            auto *params = reinterpret_cast<NCCALCSIZE_PARAMS *>(msg->lParam);
            if (IsZoomed(hwnd)) {
                MONITORINFO mi{};
                mi.cbSize = sizeof(MONITORINFO);
                GetMonitorInfoW(MonitorFromRect(&params->rgrc[0], MONITOR_DEFAULTTONEAREST), &mi);
                params->rgrc[0] = mi.rcWork;
            }
            *result = 0;
            return true;
        }
        break;

    case WM_NCHITTEST: {
        const QPoint screenPos(GET_X_LPARAM(msg->lParam), GET_Y_LPARAM(msg->lParam));
        int code = hitTest(screenPos);
        if (code == HTMAXBUTTON && !m_snapLayoutsEnabled)
            code = HTCAPTION;
        *result = code;
        return true;
    }

    case WM_NCACTIVATE:
        *result = TRUE;
        return true;

    case kWMNCUAHDrawCaption:
    case kWMNCUAHDrawFrame:
        *result = 0;
        return true;

    case WM_NCMOUSEMOVE: {
        POINT cursor{};
        GetCursorPos(&cursor);
        updateHoverState(QPoint(cursor.x, cursor.y));
        TRACKMOUSEEVENT tme{};
        tme.cbSize = sizeof(TRACKMOUSEEVENT);
        tme.dwFlags = TME_NONCLIENT | TME_LEAVE;
        tme.hwndTrack = hwnd;
        TrackMouseEvent(&tme);
        break;
    }

    case WM_NCMOUSELEAVE:
        clearHoverState();
        break;

    case WM_NCLBUTTONDOWN: {
        if (isCaptionHitCode(int(msg->wParam))) {
            POINT cursor{};
            GetCursorPos(&cursor);
            setPressedButton(buttonAt(QPoint(cursor.x, cursor.y)));
            *result = 0;
            return true;
        }
        break;
    }

    case WM_NCLBUTTONUP: {
        if (isCaptionHitCode(int(msg->wParam))) {
            POINT cursor{};
            GetCursorPos(&cursor);
            WindowButton *button = buttonAt(QPoint(cursor.x, cursor.y));
            const bool clicked = button && button == m_pressedButton && button->isPressed();
            setPressedButton(nullptr);
            if (clicked) {
                button->setHovered(true);
                QPointer<WindowButton> guard(button);
                QMetaObject::invokeMethod(this, [guard] {
                    if (guard)
                        guard->emitClicked();
                }, Qt::QueuedConnection);
            }
            *result = 0;
            return true;
        }
        break;
    }

    case WM_NCLBUTTONDBLCLK:
        if (isCaptionHitCode(int(msg->wParam))) {
            setPressedButton(nullptr);
            *result = 0;
            return true;
        }
        break;

    case WM_CANCELMODE:
        clearHoverState();
        break;

    case WM_SIZE:
        if (msg->wParam == SIZE_MAXIMIZED || msg->wParam == SIZE_RESTORED) {
            applyDwmAttributes();
            syncMaximizedState();
        }
        break;

    case WM_SETTINGCHANGE:
        applyDwmAttributes();
        break;

    default:
        break;
    }

#endif
    return QQuickWindow::nativeEvent(eventType, message, result);
}

#endif // Q_OS_WIN

FramelessWindowAttached::FramelessWindowAttached(QObject *parent)
    : QObject(parent)
{
}

bool FramelessWindowAttached::isDragRegion() const
{
    return m_dragRegion;
}

void FramelessWindowAttached::setDragRegion(bool on)
{
    if (m_dragRegion == on)
        return;
    m_dragRegion = on;
    emit dragRegionChanged();
}

bool FramelessWindowAttached::isClientArea() const
{
    return m_clientArea;
}

void FramelessWindowAttached::setClientArea(bool on)
{
    if (m_clientArea == on)
        return;
    m_clientArea = on;
    emit clientAreaChanged();
}
