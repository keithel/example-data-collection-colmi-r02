#ifndef RINGCONNECTOR_H
#define RINGCONNECTOR_H

#include <QObject>
#include <QBluetoothDeviceDiscoveryAgent>
#include <QLowEnergyController>
#include <QLowEnergyService>
#include <QTimer>
#include <qqmlintegration.h>

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

public:
    explicit RingConnector(QObject *parent = nullptr);
    ~RingConnector();

    inline bool allowAutoreconnect() const { return m_allowAutoreconnect; }
    void setAllowAutoreconnect(bool newAllowAutoreconnect);

public slots:
    void startDeviceDiscovery();
    void stopDeviceDiscovery();

signals:
    void accelerometerDataReady(qreal x, qreal y, qreal z);

    void statusUpdate(const QString &message);
    void error(const QString &message);

    void allowAutoreconnectChanged();

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
};

#endif // RINGCONNECTOR_H
