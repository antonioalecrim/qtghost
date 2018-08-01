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

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QLibrary>
#include "qtghost.h"


/**
  \brief starts ghost library if present.
  \param app Application reference.
  \param engine QML Engine reference.
  \param qtghost test library result.
*/
void ghostfy(QGuiApplication *app, QQmlApplicationEngine *engine, QtghostInterface *qtghost)
{
    const quint16 port = 35255;
    QLibrary library("qtghost");

    if (library.load()) {
        qDebug() << "Qtghost loaded";

        typedef Qtghost* (*create_Qtghost)(QGuiApplication *app, QQmlApplicationEngine *engine);
        create_Qtghost create_qtghost = (create_Qtghost)library.resolve("create_Qtghost");

         if (create_qtghost) {
             qtghost = create_qtghost(app, engine);
             if (qtghost) {
                 qtghost->init(port);
                 qDebug() << "Ghost mode initiated";
             }
         }
    }
    else {
        qDebug() << library.errorString();
    }
}

int main(int argc, char *argv[])
{
    //QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

    QGuiApplication app(argc, argv);

    QQmlApplicationEngine engine;
    engine.load(QUrl(QStringLiteral("qrc:/main.qml")));
    if (engine.rootObjects().isEmpty())
        return -1;

    QtghostInterface *qtghost = nullptr;
    ghostfy(&app, &engine, qtghost);//init Qtghost server

    return app.exec();
}
