#include "ringconnector.h"
#include <QCoreApplication>
#include <QDebug>
#include <QDataStream>

RingConnector::RingConnector(QObject *parent)
    : QObject(parent),
    m_discoveryAgent(new QBluetoothDeviceDiscoveryAgent(this)),
    m_controller(nullptr),
    m_uartService(nullptr)
{
    connect(m_discoveryAgent, &QBluetoothDeviceDiscoveryAgent::deviceDiscovered,
            this, &RingConnector::deviceDiscovered);
    connect(m_discoveryAgent, &QBluetoothDeviceDiscoveryAgent::finished,
            this, &RingConnector::deviceDiscoveryFinished);
    connect(m_discoveryAgent, &QBluetoothDeviceDiscoveryAgent::errorOccurred,
            [this](QBluetoothDeviceDiscoveryAgent::Error error) {
                if (error != QBluetoothDeviceDiscoveryAgent::NoError) {
                    emit this->error(QString("Device discovery error: %1").arg(error));
                }
            });
}

RingConnector::~RingConnector()
{
    if (m_controller) {
        m_controller->disconnectFromDevice();
        delete m_controller;
    }
}

void RingConnector::startDeviceDiscovery()
{
    emit statusUpdate("Starting device discovery...");
    m_discoveryAgent->start(QBluetoothDeviceDiscoveryAgent::LowEnergyMethod);
}

void RingConnector::deviceDiscovered(const QBluetoothDeviceInfo &device)
{
    if (device.coreConfigurations() & QBluetoothDeviceInfo::LowEnergyCoreConfiguration) {
        if (device.name().startsWith(RING_NAME_PREFIX)) {
            emit statusUpdate(QString("Found Ring: %1 (%2)").arg(device.name(), device.address().toString()));
            m_ringDevice = device;
            m_discoveryAgent->stop();

            if (m_controller) {
                m_controller->disconnectFromDevice();
                delete m_controller;
            }

            m_controller = QLowEnergyController::createCentral(m_ringDevice, this);

            connect(m_controller, &QLowEnergyController::connected,
                    this, &RingConnector::controllerConnected);
            connect(m_controller, &QLowEnergyController::errorOccurred,
                    this, &RingConnector::controllerError);
            connect(m_controller, &QLowEnergyController::disconnected,
                    this, &RingConnector::controllerDisconnected);
            connect(m_controller, &QLowEnergyController::serviceDiscovered,
                    this, &RingConnector::serviceDiscovered);
            connect(m_controller, &QLowEnergyController::discoveryFinished,
                    this, &RingConnector::serviceDiscoveryFinished);

            emit statusUpdate("Connecting to ring...");
            m_controller->connectToDevice();
        }
    }
}

void RingConnector::deviceDiscoveryFinished()
{
    if (m_ringDevice.isValid()) {
        emit statusUpdate("Device discovery finished.");
    } else {
        emit error("Device discovery finished: No ring found.");
    }
}

void RingConnector::controllerConnected()
{
    emit statusUpdate("Controller connected. Discovering services...");
    m_controller->discoverServices();
}

void RingConnector::controllerError(QLowEnergyController::Error newError)
{
    emit error(QString("Controller error: %1").arg(newError));
}

void RingConnector::controllerDisconnected()
{
    emit statusUpdate("Controller disconnected.");

    // TODO: Auto-reconnect logic
}

void RingConnector::serviceDiscovered(const QBluetoothUuid &gatt)
{
    if (gatt == UART_SERVICE_UUID) {
        emit statusUpdate("UART Service found.");
        m_uartService = m_controller->createServiceObject(UART_SERVICE_UUID, this);
        if (!m_uartService) {
            emit error("Failed to create service object.");
            return;
        }

        connect(m_uartService, &QLowEnergyService::stateChanged,
                this, &RingConnector::serviceStateChanged);
        connect(m_uartService, &QLowEnergyService::characteristicChanged,
                this, &RingConnector::characteristicChanged);

        m_uartService->discoverDetails();
    }
}

void RingConnector::serviceDiscoveryFinished()
{
    emit statusUpdate("Service discovery finished.");
    if (!m_uartService) {
        emit error("UART service not found.");
    }
}

void RingConnector::serviceStateChanged(QLowEnergyService::ServiceState newState)
{
    if (newState == QLowEnergyService::RemoteServiceDiscovered) {
        emit statusUpdate("UART Service details discovered.");

        // Find RX and TX characteristics
        m_rxCharacteristic = m_uartService->characteristic(UART_RX_CHAR_UUID);
        m_txCharacteristic = m_uartService->characteristic(UART_TX_CHAR_UUID);

        if (!m_rxCharacteristic.isValid()) {
            emit error("RX Characteristic not found.");
        } else {
            m_foundRxChar = true;
            emit statusUpdate("RX Characteristic found.");
        }

        if (!m_txCharacteristic.isValid()) {
            emit error("TX Characteristic not found.");
        } else {
            m_foundTxChar = true;
            emit statusUpdate("TX Characteristic found.");
        }

        // Continue if both found
        if (m_foundRxChar && m_foundTxChar) {

            QLowEnergyDescriptor cccd = m_txCharacteristic.descriptor(QBluetoothUuid::DescriptorType::ClientCharacteristicConfiguration);
            if (cccd.isValid()) {
                emit statusUpdate("Subscribing to TX notifications...");
                m_uartService->writeDescriptor(cccd, QByteArray::fromHex("0100")); // 0100 to enable notifications
            } else {
                emit error("CCCD not found for TX characteristic.");
                return;
            }

            // from Python ENABLE_RAW_SENSOR_CMD = create_command("a104")
            // Structure [0xA1, 0x04, ... 0x00 (padding) ..., CHECKSUM]
            QByteArray commandPacket(16, 0x00);
            commandPacket[0] = static_cast<char>(0xA1);
            commandPacket[1] = static_cast<char>(0x04);

            // Calculate and append the checksum
            // left(15) gives us the first 15 bytes (0..14)
            commandPacket[15] = calculateChecksum(commandPacket.left(15));

            emit statusUpdate(QString("Writing 'Start Stream' command (0xA104): %1").arg(commandPacket.toHex()));
            writeToRxCharacteristic(commandPacket);
        }
    }
}

void RingConnector::characteristicChanged(const QLowEnergyCharacteristic &characteristic, const QByteArray &value)
{
    if (characteristic.uuid() == UART_TX_CHAR_UUID) {
        // qInfo() << "Raw data received:" << value.toHex();
        parseAccelerometerPacket(value);
    }
}

void RingConnector::writeToRxCharacteristic(const QByteArray &data)
{
    if (!m_uartService || !m_rxCharacteristic.isValid()) {
        emit error("Cannot write, RX characteristic not valid.");
        return;
    }
    m_uartService->writeCharacteristic(m_rxCharacteristic, data, QLowEnergyService::WriteWithoutResponse);
}

qint8 RingConnector::calculateChecksum(const QByteArray &data)
{
    // Checksum is sum of first 15 bytes, mod 255
    // Python: checksum = sum(bytes_array) & 0xFF
    quint16 sum = 0;
    for(char byte : data) {
        sum += static_cast<quint8>(byte);
    }
    return static_cast<qint8>(sum & 0xFF);
}

void RingConnector::parseAccelerometerPacket(const QByteArray &packet)
{
    // qInfo() << "Parsing packet:" << packet.toHex();
    if (packet.isEmpty()) {
        return;
    }

    // Packet structure is [CMD, PAYLOAD(14), CHECKSUM]
    // What is the format and offset of X, Y, Z data - not sure if this is correct.

    const char ACCEL_PACKET_CMD = 0xA1;

    if (packet.at(0) == ACCEL_PACKET_CMD) {

        // Check if packet is long enough for 3x qint16
        if (packet.length() < (1 + 6)) { // 1-byte cmd + 6-bytes data
            qWarning() << "Accel packet too short:" << packet.length();
            return;
        }

        // Skip the command
        QDataStream stream(packet.mid(1));
        stream.setByteOrder(QDataStream::LittleEndian); // GATT fields usually are little endian

        qint16 x, y, z;
        stream >> x >> y >> z;

        if (stream.status() != QDataStream::Ok) {
            qWarning() << "Failed to parse accelerometer data.";
        } else {
            // qInfo() << "Accel:" << x << y << z;
            emit accelerometerDataReady(x, y, z);
        }
    }
}
