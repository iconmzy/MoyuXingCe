#pragma once

#include <QLoggingCategory>

class QString;

namespace kgl::infrastructure {

class AppLogger {
public:
    static void install();

private:
    static void messageHandler(QtMsgType type, const QMessageLogContext& context, const QString& message);
};

} // namespace kgl::infrastructure
