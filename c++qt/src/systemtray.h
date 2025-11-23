#ifndef SYSTEMTRAY_H
#define SYSTEMTRAY_H

#include <QObject>
#include <qqmlintegration.h>
#include <QQuickItem>
#include <QSystemTrayIcon>
#include <QQuickItemGrabResult>

class QAction;

class SystemTray : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(bool available READ available CONSTANT FINAL)
    Q_PROPERTY(QString toolTip READ toolTip WRITE setToolTip NOTIFY toolTipChanged FINAL)
    Q_PROPERTY(bool visible READ visible WRITE setVisible NOTIFY visibleChanged FINAL)

public:
    explicit SystemTray(QObject *parent = nullptr);
    ~SystemTray();

    bool available() const;
    QString toolTip() const;
    bool visible() const;
    void setToolTip(const QString &toolTip);
    void setVisible(bool visible);

signals:
    void toolTipChanged();
    void visibleChanged();
    void activated();
    void quitTriggered();
    void showDetailsRequested();

public slots:
    void updateIcon(QQuickItem *item);
    void showMessage(const QString &title, const QString &msg, int duration = DEFAULT_MESSAGE_DURATION);
    void show();
    void hide();

private slots:
    void onGrabReady();

private:
    static const int DEFAULT_MESSAGE_DURATION = 3000;
    QSystemTrayIcon *m_trayIcon = nullptr;
    QSharedPointer<QQuickItemGrabResult> m_grabResult;
    QAction *m_toolTipDisplayAction;
};

#endif // SYSTEMTRAY_H
