#include "ringconnector.h"
#include <QCoreApplication>
#include <QDebug>
#include <QDataStream>
#include <QGuiApplication>
#include <QScreen>
#include <QThread>

RingConnector::RingConnector(QObject *parent)
    : QObject(parent),
    m_discoveryAgent(new QBluetoothDeviceDiscoveryAgent(this)),
    m_controller(nullptr),
    m_uartService(nullptr),
    m_batteryRequestTimer(new QTimer(this)),
    m_packetRateTimer(new QTimer(this))
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

    m_batteryRequestTimer->setInterval(30000);
    m_batteryRequestTimer->setSingleShot(false);
    connect(m_batteryRequestTimer, &QTimer::timeout, this, &RingConnector::getBatteryLevel);

    connect(m_packetRateTimer, &QTimer::timeout, this, &RingConnector::updatePacketRate);
    m_packetRateTimer->start(5000);
}

RingConnector::~RingConnector()
{
    disableStream();
    stopDeviceDiscovery();
}

void RingConnector::startDeviceDiscovery()
{
    if (m_controller)
        stopDeviceDiscovery();

    emit statusUpdate("Starting device discovery...");
    m_discoveryAgent->start(QBluetoothDeviceDiscoveryAgent::LowEnergyMethod);
}

void RingConnector::stopDeviceDiscovery()
{
    if (m_discoveryAgent->isActive())
        m_discoveryAgent->stop();

    if (m_uartService) {
        delete m_uartService;
        m_uartService = nullptr;
    }

    // We will manage controller cleanup.
    if (m_controller) {
        m_controller->setParent(nullptr);
        if (m_controller->state() == QLowEnergyController::UnconnectedState) {
            m_controller->deleteLater();
        }
        else {
            disconnect(m_controllerDisconnectedConnection);
            connect(m_controller, &QLowEnergyController::disconnected,
                    m_controller, &QObject::deleteLater);
            m_controller->disconnectFromDevice();
        }
        m_controller = nullptr;
    }

    m_ringDevice = QBluetoothDeviceInfo();
    m_foundRxChar = false;
    m_foundTxChar = false;

    emit statusUpdate("Stopped.");
}

void RingConnector::calibrate()
{
    m_offsetAccel = m_lastRawAccel;
    emit statusUpdate("Calibrated: Zero point set.");
    qInfo() << "Calibrated offsets ->" << m_offsetAccel;
    emit accelerometerDataReady(QVector3D());
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
            m_controllerDisconnectedConnection = connect(
                    m_controller, &QLowEnergyController::disconnected,
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
    if (m_allowAutoreconnect) {
        emit statusUpdate("Controller disconnected, reconnecting.");

        // Auto-reconnect logic
        QTimer::singleShot(1000, [this](){
            startDeviceDiscovery();
        });
    }
    else {
        emit statusUpdate("Controller disconnected.");
    }
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

            if (!m_batteryRequestTimer->isActive()) {
                // Request battery level immediately, and start a timer that will repeatedly request the battery level.
                // See constructor for interval and connection.
                getBatteryLevel();
                m_batteryRequestTimer->start();
            }
        }
    }
}

void RingConnector::characteristicChanged(const QLowEnergyCharacteristic &characteristic, const QByteArray &value)
{
    if (characteristic.uuid() == UART_TX_CHAR_UUID) {
        // qInfo() << "Raw data received:" << value.toHex();
        parsePacket(value);
    }
}

void RingConnector::getBatteryLevel()
{
    if (!m_foundRxChar || !m_uartService) {
        emit error("Cannot get battery: Not connected.");
        return;
    }

    // Command: 0x03 (Battery Request)
    QByteArray commandPacket(16, 0x00);
    commandPacket[0] = static_cast<char>(0x03);
    commandPacket[15] = calculateChecksum(commandPacket.left(15));

    // emit statusUpdate("Requesting Battery Level...");
    writeToRxCharacteristic(commandPacket);
}

void RingConnector::updatePacketRate()
{
    if (m_packetRate != m_packetCounter) {
        m_packetRate = m_packetCounter / (m_packetRateTimer->interval()/1000);
        qDebug().noquote().nospace() << "Packet rate: " << m_packetRate << " Hz";
        emit packetRateChanged();
    }
    m_packetCounter = 0;
}

void RingConnector::disableStream()
{
    if (m_uartService && m_rxCharacteristic.isValid() && m_controller
        && m_controller->state() == QLowEnergyController::DiscoveredState) {
        qDebug() << "Sending Disable Stream command";
        QByteArray disablePacket(16, 0x00);
        disablePacket[0] = 0xA1;
        disablePacket[1] = 0x02;
        disablePacket[15] = calculateChecksum(disablePacket.left(15));
        writeToRxCharacteristic(disablePacket);
        emit statusUpdate("Sent Disable Stream command.");

        // Sleep for 100ms to give BLE stack a chance to actually transmit the
        // packet before we rip the connection down.
        QThread::msleep(100);
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

char RingConnector::calculateChecksum(const QByteArray &data)
{
    // Checksum is sum of first 15 bytes, mod 255
    // Python: checksum = sum(bytes_array) & 0xFF
    quint8 sum = 0;
    for(char byte : data) {
        sum += static_cast<quint8>(byte);
    }
    return static_cast<char>(sum);
}

void RingConnector::parsePacket(const QByteArray &packet)
{
    if (packet.length() < 3) return;

    m_packetCounter++;

    // Packet structure  for ACCEL_PACKET_CMD is [CMD, PAYLOAD(14), CHECKSUM]

    const quint8 ACCEL_PACKET_CMD = 0xA1;
    const quint8 BATT_PACKET_CMD = 0x03;
    const quint8 cmd = static_cast<quint8>(packet[0]);


    if (cmd == ACCEL_PACKET_CMD) {
        if (packet.length() < 10) return;

        const quint8 DESIRED_SUBTYPE = 0x03;
        const quint8 subtype = static_cast<quint8>(packet[1]);
        if (subtype != DESIRED_SUBTYPE) return;

        // Helper to extract a 12-bit value from 2 bytes (High, Low)
        auto parse12Bit = [](quint8 h, quint8 l) -> int {
            int val = (h << 4) | (l & 0xF);
            if (h & 0x08) {
                val -= (1 << 11); // 2048
            }
            return val;
        };

        quint8 accelBytes[6];
        for(auto i = 0; i < 6; i++)
            accelBytes[i] = static_cast<quint8>(packet[i+2]);

        QVector3D accelVals;
        for(auto i = 0; i < 3; i++)
            accelVals[i] = parse12Bit(accelBytes[i*2], accelBytes[i*2+1]);

        m_lastRawAccel = accelVals;

        // Apply tare offset to the values we send out.
        accelVals -= m_offsetAccel;

        // Handle Mouse Logic (if enabled)
        if (m_mouseControlEnabled) {
            handleMouseMovement(accelVals);
        }

        emit accelerometerDataReady(accelVals);
        qDebug() << "Accel Vals:" << accelVals;
    }
    // --- Battery Data ---
    else if (cmd == BATT_PACKET_CMD) {
        // Based on tahnok/colmi_r02_client battery.py
        // Packet: [0x03, level, voltage_h, voltage_l, ..., checksum]
        if (packet.length() >= 4) {
            quint8 level = static_cast<quint8>(packet[1]);
            // Voltage is likely big-endian or little-endian uint16.
            // Usually battery voltage is around 3000-4200mV
            quint8 v_h = static_cast<quint8>(packet[2]);
            quint8 v_l = static_cast<quint8>(packet[3]);

            // Assuming Big Endian for now based on typical protocols, but verify if values look wrong
            int voltage = (v_h << 8) | v_l;

            qInfo() << "[BAT STATUS]" << level << "%" << voltage << "mV";
            if (m_batteryLevel != level) {
                m_batteryLevel = level;
                emit batteryLevelChanged();
            }
            if (m_batteryVoltage != voltage) {
                m_batteryVoltage = voltage;
                emit batteryVoltageChanged();
            }
            // emit statusUpdate(QString("Battery: %1% (%2 mV)").arg(level).arg(voltage));
        }
    }
}

void RingConnector::setAllowAutoreconnect(bool newAllowAutoreconnect)
{
    if (m_allowAutoreconnect == newAllowAutoreconnect)
        return;
    m_allowAutoreconnect = newAllowAutoreconnect;
    emit allowAutoreconnectChanged();
}

void RingConnector::setMouseControlEnabled(bool enabled)
{
    if (m_mouseControlEnabled != enabled) {
        m_mouseControlEnabled = enabled;
        emit mouseControlEnabledChanged();
        if (enabled)
            emit statusUpdate("Mouse Control ENABLED");
        else
            emit statusUpdate("Mouse Control DISABLED");
    }
}

void RingConnector::handleMouseMovement(QVector3D accelVector)
{
    int x = accelVector.x();
    int y = accelVector.y();

    // 1. Deadzone check
    if (std::abs(x) < DEADZONE) x = 0;
    else x = (x > 0) ? x - DEADZONE : x + DEADZONE;

    if (std::abs(y) < DEADZONE) y = 0;
    else y = (y > 0) ? y - DEADZONE : y + DEADZONE;

    // 2. Move cursor if there is significant input
    if (x != 0 || y != 0) {
        QPoint currentPos = QCursor::pos();

        // Apply sensitivity and direct mapping
        // X -> X (Roll Right = Mouse Right)
        // Y -> Y (Pitch Down = Mouse Down)
        int dx = static_cast<int>(x * SENSITIVITY);
        int dy = static_cast<int>(y * SENSITIVITY);

        QCursor::setPos(currentPos.x() + dx, currentPos.y() + dy);
    }

    // TODO: Click detection using 'z' axis jerk
}
