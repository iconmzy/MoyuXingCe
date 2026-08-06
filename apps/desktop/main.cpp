#include "application/controllers/MainController.h"
#include "infrastructure/logging/AppLogger.h"
#include "presentation/views/MainWindow.h"

#include <QApplication>
#include <QDebug>

int main(int argc, char* argv[])
{
    QApplication application(argc, argv);
    QApplication::setOrganizationName(QStringLiteral("KeepGongLearning"));
    QApplication::setApplicationName(QStringLiteral("KeepGongLearning"));
    QApplication::setApplicationVersion(QStringLiteral("0.3.0"));
    kgl::infrastructure::AppLogger::install();
    qInfo() << "application started";

    kgl::application::MainController controller;
    kgl::presentation::MainWindow window(&controller);
    controller.initialize();
    window.show();
    return application.exec();
}
