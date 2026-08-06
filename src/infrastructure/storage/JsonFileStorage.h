#pragma once

#include <QJsonDocument>
#include <QString>

namespace kgl::infrastructure {

class JsonFileStorage {
public:
    static QString dataDirectory();
    static bool ensureDataDirectory(QString* error = nullptr);
    static bool read(const QString& filePath, QJsonDocument* document, QString* error = nullptr);
    static bool write(const QString& filePath, const QJsonDocument& document, QString* error = nullptr);
};

} // namespace kgl::infrastructure
