/**
 * @file    main.cpp
 * @brief   Punto de entrada de la aplicacion HMI para la cinta clasificadora.
 * @author  Luciano
 * @date    Mayo 2026
 */

#include <QApplication>
#include <QStyleFactory>
#include "mainwindow.h"

int main(int argc, char *argv[]) {
    QApplication::setHighDpiScaleFactorRoundingPolicy(
        Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
    QApplication app(argc, argv);
    app.setApplicationName("CintaHMI");
    app.setApplicationVersion("1.0");

    MainWindow window;
    window.show();

    return app.exec();
}
