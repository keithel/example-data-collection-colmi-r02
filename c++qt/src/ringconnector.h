#ifndef RINGCONNECTOR_H
#define RINGCONNECTOR_H

#include <QObject>
#include <QBluetoothDeviceDiscoveryAgent>
#include <QLowEnergyController>
#include <QLowEnergyService>
#include <QTimer>
#include <qqmlintegration.h>
#include <QVector3D>
#include <QCursor> // Added for mouse control
#include <QPoint>

// UUIDs from ring.py
const QBluetoothUuid UART_SERVICE_UUID(QStringLiteral("6E40FFF0-B5A3-F393-E0A9-E50E24DCCA9E"));
const QBluetoothUuid UART_RX_CHAR_UUID(QStringLiteral("6E400002-B5A3-F393-E0A9-E50E24DCCA9E")); // Write to this
const QBluetoothUuid UART_TX_CHAR_UUID(QStringLiteral("6E400003-B5A3-F393-E0A9-E50E24DCCA9E")); // Subscribe to this

const QString RING_NAME_PREFIX = "R02";

class RingConnector : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(bool allowAutoreconnect READ allowAutoreconnect WRITE setAllowAutoreconnect NOTIFY allowAutoreconnectChanged FINAL)
    Q_PROPERTY(bool mouseControlEnabled READ mouseControlEnabled WRITE setMouseControlEnabled NOTIFY mouseControlEnabledChanged)

    // --- Added Tuning Properties ---
    Q_PROPERTY(double rotation READ rotation WRITE setRotation NOTIFY rotationChanged)
    Q_PROPERTY(double sensitivity READ sensitivity WRITE setSensitivity NOTIFY sensitivityChanged)
    Q_PROPERTY(int deadzone READ deadzone WRITE setDeadzone NOTIFY deadzoneChanged)
    Q_PROPERTY(double smoothing READ smoothing WRITE setSmoothing NOTIFY smoothingChanged)

public:
    explicit RingConnector(QObject *parent = nullptr);
    ~RingConnector();

    inline bool allowAutoreconnect() const { return m_allowAutoreconnect; }
    bool mouseControlEnabled() const { return m_mouseControlEnabled; }

    // Property Getters
    double rotation() const { return m_rotation; }
    double sensitivity() const { return m_sensitivity; }
    int deadzone() const { return m_deadzone; }
    double smoothing() const { return m_smoothing; }

public slots:
    void startDeviceDiscovery();
    void stopDeviceDiscovery();
    void calibrate();

    void setAllowAutoreconnect(bool newAllowAutoreconnect);
    void setMouseControlEnabled(bool enabled);

    // Property Setters
    void setRotation(double angleDegrees);
    void setSensitivity(double val);
    void setDeadzone(int val);
    void setSmoothing(double alpha);

signals:
    void accelerometerDataReady(QVector3D accelVector);
    void statusUpdate(const QString &message);
    void error(const QString &message);
    void allowAutoreconnectChanged();
    void mouseControlEnabledChanged();

    // Property Notifiers
    void rotationChanged();
    void sensitivityChanged();
    void deadzoneChanged();
    void smoothingChanged();

private slots:
    // Device discovery slots
    void deviceDiscovered(const QBluetoothDeviceInfo &device);
    void deviceDiscoveryFinished();

    // QLowEnergyController slots
    void controllerConnected();
    void controllerError(QLowEnergyController::Error newError);
    void controllerDisconnected();

    // Service discovery slots
    void serviceDiscovered(const QBluetoothUuid &gatt);
    void serviceDiscoveryFinished();

    // QLowEnergyService slots
    void serviceStateChanged(QLowEnergyService::ServiceState newState);
    void characteristicChanged(const QLowEnergyCharacteristic &characteristic, const QByteArray &value);

private:
    void writeToRxCharacteristic(const QByteArray &data);
    char calculateChecksum(const QByteArray &data);
    void parseAccelerometerPacket(const QByteArray &packet);
    void handleMouseMovement(QVector3D accelVector);

private:
    QMetaObject::Connection m_controllerDisconnectedConnection;

    QBluetoothDeviceDiscoveryAgent *m_discoveryAgent = nullptr;
    QLowEnergyController *m_controller = nullptr;
    QLowEnergyService *m_uartService = nullptr;
    QLowEnergyCharacteristic m_rxCharacteristic;
    QLowEnergyCharacteristic m_txCharacteristic;

    QBluetoothDeviceInfo m_ringDevice;
    bool m_foundRxChar = false;
    bool m_foundTxChar = false;
    bool m_allowAutoreconnect = false;

    // Storage for calibration
    QVector3D m_lastRawAccel;
    QVector3D m_offsetAccel;

    bool m_mouseControlEnabled = false;

    // --- Tuning Configuration ---
    // Changed from const to member variables for runtime tuning
    int m_deadzone = 200;
    double m_sensitivity = 0.015;
    double m_rotation = 0.0;     // Degrees
    double m_smoothing = 0.5;    // Alpha (0.0 - 1.0)

    // Smoothing State
    double m_smoothX = 0;
    double m_smoothY = 0;
};

#endif // RINGCONNECTOR_H
