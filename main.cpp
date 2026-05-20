/**
 * @file    main.cpp
 * @brief   Punto de entrada de la aplicacion HMI para la cinta clasificadora.
 *
 * @details Configura el soporte de High DPI para pantallas con escalado
 *          fraccionario (125%, 150%, etc.) y lanza la ventana principal
 *          del HMI.
 *
 *          La politica PassThrough permite que Qt escale correctamente
 *          en cualquier monitor sin importar la resolucion o el factor
 *          de escala del sistema operativo.
 *
 * @author  Luciano
 * @date    Mayo 2026
 */

#include <QApplication>
#include <QStyleFactory>
#include "mainwindow.h"

/**
 * @brief Punto de entrada principal de la aplicacion.
 *
 * @details Inicializa QApplication con soporte High DPI, crea la ventana
 *          principal y entra en el event loop de Qt.
 *
 * @param argc Cantidad de argumentos de linea de comandos.
 * @param argv Arreglo de argumentos de linea de comandos.
 * @return Codigo de salida de la aplicacion (0 = exito).
 */
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
