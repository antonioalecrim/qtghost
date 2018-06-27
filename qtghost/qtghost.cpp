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
#include <QQuickWindow>
#include <QDir>
#include <QBuffer>

Qtghost::Qtghost(QGuiApplication *app, QQmlApplicationEngine *engine)
{
    keyPressed = false;
    allMouseMoves = false;
    recording = false;
    stepbystep = false;

    app->installEventFilter(this);
    appI = app;
    eng = engine;
    eventsIndex = 0;
    createScreenshotCache = false;
    toWatch = eng->rootObjects()[0];

    connect(&playTimer,SIGNAL(timeout()),this,SLOT(consume_event()));
}

QString Qtghost::getVersion()
{
    return QString(VERSION);
}

void Qtghost::setWatchable(QObject *watch)
{
    toWatch = watch;
}

bool Qtghost::eventFilter(QObject *watched, QEvent * event)
{
    QMouseEvent *mouseEvent;
    QDropEvent *genericDragEvent;
    QTouchEvent *touchEvent;
    QKeyEvent *keyEvent;
    QWheelEvent *wheelEvent;
    QList<QTouchEvent::TouchPoint> touchList;

    if (watched == toWatch) {
        switch(event->type()) {
        case QEvent::MouseButtonPress:
        case QEvent::MouseButtonRelease:
        case QEvent::MouseButtonDblClick:
            mouseEvent = static_cast<QMouseEvent*>(event);
            add_event(mouseEvent->pos(), event->type());
            keyPressed = (event->type() == QEvent::MouseButtonPress);
            break;
        case QEvent::MouseMove:
            if (keyPressed || allMouseMoves) {
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
        case QEvent::KeyPress:
        case QEvent::KeyRelease:
        case QEvent::ShortcutOverride:
            keyEvent = static_cast<QKeyEvent*>(event);
            add_event(QPointF(),
                      event->type(),
                      keyEvent->key(),
                      keyEvent->text());
            break;
        case QEvent::Wheel:
            wheelEvent = static_cast<QWheelEvent*>(event);
            add_event(wheelEvent->pos(), event->type(), wheelEvent->delta(),
                      QString::number(wheelEvent->orientation()),
                      wheelEvent->globalPosF());
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
    QKeyEvent *keyEvent;
    QWheelEvent *wheelEvent;
    Qt::Orientation orientation;

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
            appI->sendEvent(toWatch, eve);
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
            appI->sendEvent(toWatch, genericDragEvent);
            delete genericDragEvent;
            break;
        case QEvent::KeyPress:
        case QEvent::KeyRelease:
        case QEvent::ShortcutOverride:
            keyEvent = new QKeyEvent(events[eventsIndex].type,
                                     events[eventsIndex].argI,
                                     Qt::NoModifier,
                                     events[eventsIndex].argS);
            appI->sendEvent(toWatch, keyEvent);
            delete keyEvent;
            break;
        case QEvent::Wheel:
            orientation = (Qt::Orientation)events[eventsIndex].argS.toInt();
            wheelEvent = new QWheelEvent(events[eventsIndex].pos,
                                         events[eventsIndex].pos2,
                                         QPoint(),
                                         (orientation == Qt::Vertical) ?
                                             QPoint(0,events[eventsIndex].argI) :
                                             QPoint(events[eventsIndex].argI, 0),
                                         events[eventsIndex].argI,
                                         orientation,
                                         Qt::NoButton,
                                         Qt::NoModifier);
            appI->sendEvent(toWatch, wheelEvent);
            delete wheelEvent;
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

int Qtghost::add_event(QPointF p, QEvent::Type t, int argI, QString argS, QPointF p2)
{
    if (recording) {
        recEvent rec;
        rec.pos = p;
        rec.time = time.restart();
        rec.type = t;
        rec.argI = argI;
        rec.argS = argS;
        rec.pos2 = p2;
        events.append(rec);
    }

    return 0;
}

int Qtghost::init(quint16 port)
{
    server = new Server(this, port);
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
        //parser.addHelpOption();
        //parser.addVersionOption();
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
        QCommandLineOption getVerOption(QStringList() << "v" << "version",
                QCoreApplication::translate("version", "send version."));
        parser.addOption(getVerOption);
        QCommandLineOption getScrOption(QStringList() << "c" << "screenshot",
                QCoreApplication::translate("screenshot", "take screenshot."));
        parser.addOption(getScrOption);

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
            server->sendRec("-j ", getJSONEvents().toJson());
        if (parser.isSet(getVerOption))
            server->sendRec("-v ", QString(VERSION).toUtf8());
        if (parser.isSet(getScrOption)) {
            QQuickWindow *view = qobject_cast<QQuickWindow*>(toWatch);
            QString name = "qtghost_scr.png";
            QImage img = view->grabWindow();
            if (createScreenshotCache) {
                QString path = QDir::tempPath();
                QFile::remove(path+"/"+name);
                if (img.save(path+"/_tmp_"+name)) {
                    QFile::rename(path+"/_tmp_"+name, path+"/"+name);
                    qDebug() << "Qtghost: new screenshot at: " << path+"/"+name;
                }
                else {
                    qDebug() << "Qtghost: failed to create a file at: " << path+"/"+name;
                }
            }
            QByteArray ba;
            QBuffer buffer(&ba);
            buffer.open(QIODevice::WriteOnly);
            img.save(&buffer, "PNG");
            server->sendRec("-c ", ba);
        }
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
        event.pos = QPointF(array.at(i).toObject().value("posX").toDouble(),
                            array.at(i).toObject().value("posY").toDouble());
        event.time = array.at(i).toObject().value("time").toInt();
        event.type = static_cast<QEvent::Type>(array.at(i).toObject().value("type").toInt());
        events.append(event);
    }
    qDebug() << "Qtghost:" << "New JSON set, size: " << events.size();
}

void Qtghost::setStoreAllMouseMoves(bool flag)
{
    allMouseMoves = flag;
}
