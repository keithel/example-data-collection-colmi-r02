// Colmi R02 Qt C++ Data Explorer App
//
// Copyright (C) 2025 Keith Kyzivat <keithel @ github>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include "ringconnector.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    QQmlApplicationEngine engine;
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);
    engine.loadFromModule("R02DataExplorer", "Main");

    RingConnector connector;
    QObject::connect(&connector, &RingConnector::statusUpdate, [](const QString &message) {
        qInfo().noquote() << "[STATUS]" << message;
    });
    QObject::connect(&connector, &RingConnector::error, [](const QString &errMsg) {
        qCritical().noquote() << "[ERROR]" << errMsg;
    });
    QObject::connect(&connector, &RingConnector::accelerometerDataReady, [](qint16 x, qint16 y, qint16 z) {
        qInfo().noquote() << "[DATA] x:" << x << "y:" << y << "z:" << z;
    });
    connector.startDeviceDiscovery();

    return app.exec();
}
