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

#include "qtghost.h"
#include <QCommandLineParser>
#include <QMouseEvent>
#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

Qtghost::Qtghost(QGuiApplication *app, QQmlApplicationEngine *engine)
{
    keyPressed = false;
    recording = false;
    stepbystep = false;

    app->installEventFilter(this);
    appI = app;
    eng = engine;
    eventsIndex = 0;

    connect(&playTimer,SIGNAL(timeout()),this,SLOT(consume_event()));
}

bool Qtghost::eventFilter(QObject *watched, QEvent * event)
{
    QMouseEvent *mouseEvent;
    QDropEvent *genericDragEvent;
    QTouchEvent *touchEvent;
    QList<QTouchEvent::TouchPoint> touchList;

    if (watched == eng->rootObjects()[0]) {
        switch(event->type()) {
        case QEvent::MouseButtonPress:
        case QEvent::MouseButtonRelease:
        case QEvent::MouseButtonDblClick:
            mouseEvent = static_cast<QMouseEvent*>(event);
            add_event(mouseEvent->pos(), event->type());
            keyPressed = (event->type() == QEvent::MouseButtonPress);
            break;
        case QEvent::MouseMove:
            if (keyPressed) {
                mouseEvent = static_cast<QMouseEvent*>(event);
                add_event(mouseEvent->pos(), event->type());
            }
            break;
        case QEvent::DragEnter:
        case QEvent::DragLeave:
        case QEvent::DragMove:
        case QEvent::DragResponse:
            genericDragEvent = static_cast<QDropEvent*>(event);
            add_event(genericDragEvent->pos(), event->type());
            break;
        case QTouchEvent::TouchBegin:
        case QTouchEvent::TouchCancel:
        case QTouchEvent::TouchEnd:
            touchEvent = static_cast<QTouchEvent*>(event);
            touchList = touchEvent->touchPoints();
            if (touchList.length()) {
                // convert into mouse event for simplicity
                if (event->type() == QTouchEvent::TouchBegin) {
                    add_event(touchList[0].pos(), QEvent::MouseButtonPress);
                }
                else {
                    add_event(touchList[0].pos(), QEvent::MouseButtonRelease);
                }
            }
            keyPressed = (event->type() == QTouchEvent::TouchBegin);
            break;
        case QTouchEvent::TouchUpdate:
            if (keyPressed) {
                touchEvent = static_cast<QTouchEvent*>(event);
                touchList = touchEvent->touchPoints();
                if (touchList.length()) {
                    // convert into mouse event for simplicity
                    add_event(touchList[0].pos(), QEvent::MouseMove);
                }
            }
            break;
        default:
            break;
        }
    }

    return false;
}

void Qtghost::consume_event()
{
    QMouseEvent *eve;
    QDropEvent *genericDragEvent;

    if (eventsIndex < events.length()) {
        switch (events[eventsIndex].type) {
        case QEvent::MouseButtonPress:
        case QEvent::MouseButtonRelease:
        case QEvent::MouseButtonDblClick:
        case QEvent::MouseMove:
            eve = new QMouseEvent(events[eventsIndex].type,
                            events[eventsIndex].pos,
                            Qt::LeftButton, //should get this from event register?
                            Qt::NoButton,
                            Qt::NoModifier);
            appI->sendEvent(eng->rootObjects()[0], eve);
            delete eve;
            break;
        case QEvent::DragEnter:
        case QEvent::DragLeave:
        case QEvent::DragMove:
        case QEvent::DragResponse:
            genericDragEvent = new QDropEvent(events[eventsIndex].pos,
                                        Qt::MoveAction,
                                        Q_NULLPTR,
                                        Qt::LeftButton,
                                        Qt::NoModifier,
                                        events[eventsIndex].type);
            appI->sendEvent(eng->rootObjects()[0], genericDragEvent);
            delete genericDragEvent;
            break;
        default:
            break;
        }

        if (!stepbystep) {
            if (++eventsIndex < events.length()) { //next event
                playTimer.start(events[eventsIndex].time);
            }
            else {
                qDebug() << "Qtghost:" << "Ghost mode stopped!";
            }
        }
        else {
            stepbystep = false;
            qDebug() << "Qtghost:" << "step: one event consumed: "
                     << events[eventsIndex].type << " at "
                     << events[eventsIndex].pos;
        }
    }
}

int Qtghost::play()
{
    qDebug() << "Qtghost:" << "Running in ghost mode! size: " << events.length();
    eventsIndex = 0;
    playTimer.setSingleShot(true);
    playTimer.start(10); //just to start with something

    return 0;
}

int Qtghost::step()
{
    stepbystep = true;
    if (eventsIndex <= events.length()) {
        consume_event();
        eventsIndex++;
    }
    else {
        eventsIndex = 0;
    }

    return 0;
}

int Qtghost::record_start()
{
    events.clear();
    recording = true;
    time.start();
    qDebug() << "Qtghost:" << "Creating a ghost!";

    return 0;
}

int Qtghost::record_stop()
{
    recording = false;
    qDebug() << "Qtghost:" << "Ghost creation done!";

    return 0;
}

int Qtghost::add_event(QPointF p, QEvent::Type t)
{
    if (recording) {
        recEvent rec;
        rec.pos = p;
        rec.time = time.restart();
        rec.type = t;
        events.append(rec);
    }

    return 0;
}

int Qtghost::init()
{
    server = new Server(this);
    connect(server, SIGNAL(dataReceived(QByteArray)), SLOT(processCMD(QByteArray)));

    return 0;
}

void Qtghost::processCMD(QByteArray data)
{
    processCMD(QString(data));
}

void Qtghost::processCMD(QString cmd)
{
    if (!cmd.startsWith("-j") && !cmd.startsWith("--JSON")) {
        QStringList arguments = QString("Qtghost "+cmd).split(" ");
        QCommandLineParser parser;
        parser.setApplicationDescription("Qtghost");
        parser.addHelpOption();
        parser.addVersionOption();
        // A boolean option with multiple names (-r, --record)
        QCommandLineOption recordOption(QStringList() << "r" << "record",
                QCoreApplication::translate("record", "Start recording."));
        parser.addOption(recordOption);
        // A boolean option with multiple names (-s, --stop-recording)
        QCommandLineOption stopRecordOption(QStringList() << "s" << "stop-recording",
                QCoreApplication::translate("stop", "Stop recording."));
        parser.addOption(stopRecordOption);
        // A boolean option with multiple names (-p, --play)
        QCommandLineOption playOption(QStringList() << "p" << "play",
                QCoreApplication::translate("play", "Start playing."));
        parser.addOption(playOption);
        // A boolean option with multiple names (-e, --step)
        QCommandLineOption stepOption(QStringList() << "e" << "step",
                QCoreApplication::translate("step", "Pay one step."));
        parser.addOption(stepOption);
        // A boolean option with multiple names (-g, --get-rec)
        QCommandLineOption getRecOption(QStringList() << "g" << "get-rec",
                QCoreApplication::translate("get", "Get recorded ghost."));
        parser.addOption(getRecOption);

        // Process the actual command line arguments given by the user
        parser.process(arguments);
        if (parser.isSet(recordOption))
            record_start();
        if (parser.isSet(stopRecordOption))
            record_stop();
        if (parser.isSet(playOption))
            play();
        if (parser.isSet(stepOption))
            step();
        if (parser.isSet(getRecOption))
            server->sendRec(getJSONEvents().toJson());
    }
    else {
        bool isJSON = false;

        if (cmd.startsWith("-j ")) {
            cmd.remove(0, 3);
            isJSON = true;
        }
        else if (cmd.startsWith("-JSON ")) {
                cmd.remove(0, 6);
                isJSON = true;
        }
        if (isJSON) {
            setJSONEvents(QJsonDocument::fromJson(cmd.toUtf8()));
        }
    }
}


QJsonDocument Qtghost::getJSONEvents()
{
    QJsonObject mainObj;
    QJsonArray array;

    foreach(recEvent event, events) {
        QJsonObject obj;
        obj.insert("posX", QJsonValue(event.pos.x()).toDouble());
        obj.insert("posY", QJsonValue(event.pos.y()).toDouble());
        obj.insert("time", QJsonValue(event.time).toInt());
        obj.insert("type", QJsonValue(event.type).toInt());

        array.append(QJsonValue(obj));
    }
    mainObj.insert("events", array);

    return QJsonDocument(mainObj);
}

void Qtghost::setJSONEvents(QJsonDocument doc)
{
    QJsonObject mainObj = doc.object();
    QJsonArray array = mainObj["events"].toArray();

    events.clear();
    for (int i=0; i < array.size(); i++) {
        recEvent event;
        event.pos = QPointF(array.at(i)["posX"].toDouble(),array.at(i)["posY"].toDouble());
        event.time = array.at(i)["time"].toInt();
        event.type = static_cast<QEvent::Type>(array.at(i)["type"].toInt());
        events.append(event);
    }
    qDebug() << "Qtghost:" << "New JSON set, size: " << events.size();
}
