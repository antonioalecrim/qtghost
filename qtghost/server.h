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

#ifndef SERVER_H
#define SERVER_H

#include <QObject>
#include <QTcpServer>
#include <QNetworkSession>
#include <QDebug>

const short buffer_size = 4096;

class Server : public QObject
{
    QTcpServer *tcpServer = nullptr; ///< \brief tcp server class.
    QTcpSocket *socket; ///< \brief tcp socket.
    QNetworkSession *networkSession = nullptr; ///< \brief if a networkSession is required.

    QByteArray buffer; ///< \brief to store received data until is complete.
    qint64 bLength; ///< \brief current transfer size.
    quint16 portI; ///< \brief server port

    Q_OBJECT
public:
    /**
      \brief Server Class constructor.
      \param parent object parent.
      \param port server port, 0 value for automatic mode.
    */
    explicit Server(QObject *parent = nullptr, quint16 port = 0);
    /**
      \brief to send data to connected client.
      \param data data to send.
      \param cmd packet command.
    */
    void sendRec(QString cmd, QByteArray data);

signals:
    /**
     \brief when a new data is received from a socket.
     \param QByteArray data that was received.
    !*/
    void dataReceived(QByteArray);

private slots:
    /**
      \brief when a new session is opened.
    */
    void sessionOpened();
    /**
      \brief when a new connection is received.
    */
    void newConnection();
    /**
      \brief when there's new data to be read from a client.
    */
    void readyRead();
    /**
      \brief when a client disconnects.
    */
    void disconnected();
};

#endif // SERVER_H
