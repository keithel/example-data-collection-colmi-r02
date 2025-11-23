#include "systemtray.h"
#include <QMenu>
#include <QAction>
#include <QDebug>

SystemTray::SystemTray(QObject *parent)
    : QObject{parent}
{
    if (QSystemTrayIcon::isSystemTrayAvailable()) {
        qDebug() << "SystemTray available";
        m_trayIcon = new QSystemTrayIcon(this);
        QMenu *menu = new QMenu();
        m_toolTipDisplayAction = menu->addAction("<tooltip>");
        connect(m_toolTipDisplayAction, &QAction::triggered, this, &SystemTray::showDetailsRequested);
        QAction *quitAction = menu->addAction("&Quit");
        connect(quitAction, &QAction::triggered, this, &SystemTray::quitTriggered);
        m_trayIcon->setContextMenu(menu);

        connect(m_trayIcon, &QSystemTrayIcon::activated, this, &SystemTray::activated);
    }
    else
    {
        qDebug() << "SystemTray not available, no battery level indicator in system tray.";
    }
}

SystemTray::~SystemTray()
{
    if (m_trayIcon) {
        m_trayIcon->hide();
        delete m_trayIcon;
    }
}

bool SystemTray::available() const
{
    return QSystemTrayIcon::isSystemTrayAvailable();
}

QString SystemTray::toolTip() const
{
    return m_trayIcon ? m_trayIcon->toolTip() : QString();
}

bool SystemTray::visible() const {
    return m_trayIcon ? m_trayIcon->isVisible() : false;
}

void SystemTray::setToolTip(const QString &toolTip)
{
    if (m_trayIcon && m_trayIcon->toolTip() != toolTip) {
        m_trayIcon->setToolTip(toolTip);
        m_toolTipDisplayAction->setText(toolTip);
        m_toolTipDisplayAction->setToolTip(toolTip);
        emit toolTipChanged();
    }
}

void SystemTray::setVisible(bool visible)
{
    if (m_trayIcon) {
        if (!m_trayIcon->isVisible() && visible) {
            m_trayIcon->show();
            emit visibleChanged();
        }
        else if (m_trayIcon->isVisible() && !visible) {
            m_trayIcon->hide();
            emit visibleChanged();
        }
    }
}

void SystemTray::updateIcon(QQuickItem *item)
{
    if (!m_trayIcon || !item)
        return;

    m_grabResult = item->grabToImage();
    if (m_grabResult) {
        connect(m_grabResult.data(), &QQuickItemGrabResult::ready, this, &SystemTray::onGrabReady);
    }
}

void SystemTray::showMessage(const QString &title, const QString &msg, int duration)
{
    if (m_trayIcon)
        m_trayIcon->showMessage(title, msg, QSystemTrayIcon::Information, duration);
}

void SystemTray::show()
{
    qDebug() << "Showing tray icon";
    if (m_trayIcon)
        m_trayIcon->show();
}

void SystemTray::hide()
{
    if (m_trayIcon)
        m_trayIcon->hide();
}

void SystemTray::onGrabReady()
{
    qDebug() << "Battery grab is ready";
    if (m_grabResult && m_trayIcon) {
        m_trayIcon->setIcon(QIcon(QPixmap::fromImage(m_grabResult->image())));
        m_grabResult.clear();
    }
}
