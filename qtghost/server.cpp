/*
* MIT License
*
* Copyright (c) 2018 Antonio Alecrim Jr
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*/

#include "server.h"
#include <QtNetwork>
#include <QtCore>

Server::Server(QObject *parent) : QObject(parent)
{
    bLength = 0;
    QNetworkConfigurationManager manager;
    if (manager.capabilities() & QNetworkConfigurationManager::NetworkSessionRequired) {
        // Get saved network configuration
        QSettings settings(QSettings::UserScope, QLatin1String("QtProject"));
        settings.beginGroup(QLatin1String("QtNetwork"));
        const QString id = settings.value(QLatin1String("DefaultNetworkConfiguration")).toString();
        settings.endGroup();

        // If the saved network configuration is not currently discovered use the system default
        QNetworkConfiguration config = manager.configurationFromIdentifier(id);
        if ((config.state() & QNetworkConfiguration::Discovered) !=
                QNetworkConfiguration::Discovered) {
            config = manager.defaultConfiguration();
        }

        networkSession = new QNetworkSession(config, this);
        connect(networkSession, &QNetworkSession::opened, this, &Server::sessionOpened);

        qDebug() << "Qtghost:" << (tr("Opening network session."));
        networkSession->open();
    } else {
        sessionOpened();
    }
    tcpServer->setMaxPendingConnections(1); //by design: one client
    connect(tcpServer, &QTcpServer::newConnection, this, &Server::newConnection);
}


void Server::sessionOpened()
{
    // Save the used configuration
    if (networkSession) {
        QNetworkConfiguration config = networkSession->configuration();
        QString id;
        if (config.type() == QNetworkConfiguration::UserChoice)
            id = networkSession->sessionProperty(QLatin1String("UserChoiceConfiguration")).toString();
        else
            id = config.identifier();

        QSettings settings(QSettings::UserScope, QLatin1String("QtProject"));
        settings.beginGroup(QLatin1String("QtNetwork"));
        settings.setValue(QLatin1String("DefaultNetworkConfiguration"), id);
        settings.endGroup();
    }

    tcpServer = new QTcpServer(this);
    if (!tcpServer->listen()) {
        qDebug() << "Qtghost:" << "Unable to start the server: "+tcpServer->errorString();

        exit(1);
    }
    QString ipAddress;
    QList<QHostAddress> ipAddressesList = QNetworkInterface::allAddresses();
    // use the first non-localhost IPv4 address
    for (int i = 0; i < ipAddressesList.size(); ++i) {
        if (ipAddressesList.at(i) != QHostAddress::LocalHost &&
                ipAddressesList.at(i).toIPv4Address()) {
            ipAddress = ipAddressesList.at(i).toString();
            break;
        }
    }
    // if we did not find one, use IPv4 localhost
    if (ipAddress.isEmpty())
        ipAddress = QHostAddress(QHostAddress::LocalHost).toString();
    qDebug() << "Qtghost:" << tr("server is running on IP: %1 port: %2").arg(ipAddress).arg(tcpServer->serverPort());
}

void Server::newConnection()
{
    while (tcpServer->hasPendingConnections()) {
        socket = tcpServer->nextPendingConnection();
        connect(socket, SIGNAL(readyRead()), SLOT(readyRead()));
        connect(socket, SIGNAL(disconnected()), SLOT(disconnected()));
    }
}

void Server::readyRead()
{
    while (socket->bytesAvailable() > 0)
    {
        QByteArray tmpBuffer;
        do {
            tmpBuffer = socket->readAll();
            if (!buffer.length()) {
                int index = tmpBuffer.indexOf(":");
                bLength = QString(tmpBuffer.mid(0, index)).toInt();
                tmpBuffer.remove(0, index+1);
            }
            buffer.append(tmpBuffer);
        } while(tmpBuffer.size() > 0);

        if (buffer.length() >= bLength) {
            qDebug() << "Qtghost:" << "received length: " << buffer.length();
            emit dataReceived(buffer);
            buffer.clear();
        }
    }
}

void Server::disconnected()
{
    //qDebug() << "Qtghost:" << "client disconnected";
    bLength = 0;
    buffer.clear();
}

void Server::sendRec(QByteArray data)
{
    qint64 xfered = 0;
    int l =  data.length();
    qint64 status;

    while (data.size()) {
        if (data.size() >= 4096)
            status = socket->write(data, 4096);
        else
            status = socket->write(data, data.size());
        if (status >= 0) {
            xfered += status;
            data.remove(0, status);
        }
        else {
            qDebug() << "Qtghost:" << "error during " << __FUNCTION__;

            return;
        }
    }
    if (xfered) {
        qDebug() << "Qtghost:" << xfered << "transfered from data length " << l;
    }
}
