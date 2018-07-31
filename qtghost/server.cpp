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

Server::Server(QObject *parent, quint16 port) : QObject(parent)
{
    bLength = 0;
    portI = port;
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
    int attempts = 0;

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
    while (attempts < 2) {
        if (tcpServer->listen(QHostAddress::Any, portI)) {

            break; //if successful continue the program execution
        }
        //if failed it disables the system proxy and tries one more time.
        //exit with failure after the second attempt.
        if (attempts) {
            qDebug() << "Qtghost:" << "Unable to start the server: "+tcpServer->errorString();

            exit(1);
        }
        QNetworkProxyFactory::setUseSystemConfiguration(false);
        attempts++;
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
    qDebug() << "Qtghost:" << tr("server is running on IP: %1 port: %2 using system proxy:%3").arg(ipAddress)
                .arg(tcpServer->serverPort()).arg(attempts ? "false" : "true");
}

void Server::newConnection()
{
    while (tcpServer->hasPendingConnections()) {
        socket = tcpServer->nextPendingConnection();
        connect(socket, SIGNAL(readyRead()), SLOT(readyRead()));
        connect(socket, SIGNAL(disconnected()), SLOT(disconnected()));
    }
}

int Server::getPacketLength(QByteArray *buffer)
{
    int length = 0;
    int index = buffer->indexOf(":");

    if (index > 0) {
        length = QString(buffer->left(index)).toInt();
        buffer->remove(0, index+1);
    }

    return length;
}

void Server::readyRead()
{
    while (socket->bytesAvailable() > 0)
    {
        QByteArray tmpBuffer;
        do {
            tmpBuffer = socket->readAll();
            if (!buffer.length()) {
                bLength = getPacketLength(&tmpBuffer);
            }
            buffer.append(tmpBuffer);

            if (buffer.length() > 0 && buffer.length() >= bLength) {
                qDebug() << "Qtghost:" << "received length: " << buffer.length();
                emit dataReceived(buffer.left(bLength));
                buffer.remove(0, bLength);
                if (buffer.length() > 0) {
                    bLength = getPacketLength(&buffer);
                }
            }

        } while(tmpBuffer.size() > 0);
    }
}

void Server::disconnected()
{
    //qDebug() << "Qtghost:" << "client disconnected";
    bLength = 0;
    buffer.clear();
}

void Server::sendRec(QString cmd, QByteArray data)
{
    qint64 xfered = 0;
    int dLength =  data.length();
    qint64 status;
    QString header = QString::number(dLength)+":"+cmd;

    data.prepend(header.toStdString().c_str()); //adding header

    while (data.size()) {
        if (data.size() >= buffer_size)
            status = socket->write(data, buffer_size);
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
        qDebug() << "Qtghost:" << xfered << "transfered from data length " << dLength;
    }
}
