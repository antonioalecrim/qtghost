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

#ifndef QTGHOST_H
#define QTGHOST_H

#include <QQmlApplicationEngine>
#include <QGuiApplication>
#include <QTimer>
#include <QTime>
#include "qtghost_global.h"
#include "server.h"

///< \brief Stores a GUI event
struct recEvent {
    QPointF pos; ///< \brief position where the event occurred
    int time; ///< \brief QTime returns int for QTime::elapsed
    QEvent::Type type; ///< \brief mouse press, release, etc.
};

class QTGHOSTSHARED_EXPORT Qtghost: QObject
{
    const char* VERSION = "0.0.1"; ///< \brief Lib version.

    QGuiApplication *appI; ///< \brief Pointer to user app.
    QQmlApplicationEngine *eng; ///< \brief Pointer to user QML engine.
    bool keyPressed; ///< \brief flag to detect drag events.
    bool recording; ///< \brief if user events are being recorded.
    bool stepbystep; ///< \brief play just one event at time.
    QList<recEvent> events; ///< \brief will hold user events.
    QTime time; ///< \brief to get timestamps.
    QTimer playTimer; ///< \brief to trigger the next event while playing in ghost mode.
    QTimer updateRequestTimer; ///< \brief will force a screen refresh.
    int eventsIndex; ///< \brief to point to the current event into ghost mode play.
    Server *server; ///< \brief server to receive remote commands.
    bool createScreenshotCache; ///< \brief will create a local temp file for debug. False by default.
    Q_OBJECT

public:
    /**
      \brief class constructor.
      \param app pointer to user app.
      \param engine pointer to QML engine.
    */
    Qtghost(QGuiApplication *app, QQmlApplicationEngine *engine);
    /**
      \brief event filter to detect user events.
      \paragraph watched pointer to current filtered object's event.
      \param event current event.
      \return return a bool if the event was consumed or not.
    */
    bool eventFilter(QObject *watched, QEvent * event);
    /**
      \brief will start playing user ghost (recorded events).
      \return 0 on success.
    */
    int play();
    /**
      \brief will play just one user ghost recorded event.
      \return 0 on success.
    */
    int step();
    /**
      \brief start recording user events to play user ghost later.
      \return 0 on success.
     */
    int record_start();
    /**
      \brief stop recording user events.
      \return 0 on success.
    */
    int record_stop();
    /**
      \brief register an user event into events list to used later in ghost mode.
      \param p position where the event occurred.
      \param t event type (move, click, etc).
      \return 0 on success.
    */
    int add_event(QPointF p, QEvent::Type t);
    /**
      \brief Init the ghost mode, for now init the server.
      \return 0 on success.
    */
    int init();
    /**
      \brief process an received command.
      \param cmd command to be processed.
    */
    void processCMD(QString cmd);
    /**
      \brief will convert in memory user events into a JSON document.
      \return json document containing user events.
    */
    QJsonDocument getJSONEvents();
    /**
      \brief having a JSON document this function will put it into app memory being ready for Ghost play.
      \param doc JSON document to be inserted into ghost memory.
    */
    void setJSONEvents(QJsonDocument doc);

public slots:
    /**
      \brief when called it will consume end execute an recorded event (ghost play).
    */
    void consume_event();
    /**
      \brief called when theres data ready to be converted into Ghost command.
    */
    void processCMD(QByteArray);
};

#endif // QTGHOST_H
