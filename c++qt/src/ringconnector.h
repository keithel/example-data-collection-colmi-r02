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
    Q_PROPERTY(int batteryLevel READ batteryLevel NOTIFY batteryLevelChanged FINAL)
    Q_PROPERTY(int batteryVoltage READ batteryVoltage NOTIFY batteryVoltageChanged FINAL)

public:
    explicit RingConnector(QObject *parent = nullptr);
    ~RingConnector();

    inline bool allowAutoreconnect() const { return m_allowAutoreconnect; }
    void setAllowAutoreconnect(bool newAllowAutoreconnect);
    bool mouseControlEnabled() const { return m_mouseControlEnabled; }
    void setMouseControlEnabled(bool enabled);
    int batteryLevel() const { return m_batteryLevel; }
    int batteryVoltage() const { return m_batteryVoltage; }

public slots:
    void startDeviceDiscovery();
    void stopDeviceDiscovery();
    void calibrate();

signals:
    void accelerometerDataReady(QVector3D accelVector);
    void statusUpdate(const QString &message);
    void error(const QString &message);

    void allowAutoreconnectChanged();
    void mouseControlEnabledChanged();
    void batteryLevelChanged();
    void batteryVoltageChanged();

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

    void getBatteryLevel();

private:
    void writeToRxCharacteristic(const QByteArray &data);
    char calculateChecksum(const QByteArray &data);
    void parsePacket(const QByteArray &packet); // Renamed from parseAccelerometerPacket
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

    // Configuration
    const int DEADZONE = 200;   // Ignore movements smaller than this
    const double SENSITIVITY = 0.015; // Multiplier for cursor speed

    QTimer *m_batteryRequestTimer = nullptr;
    int m_batteryLevel = -1;
    int m_batteryVoltage = -1;
};

#endif // RINGCONNECTOR_H
