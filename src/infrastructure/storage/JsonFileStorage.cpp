#include "infrastructure/storage/JsonFileStorage.h"

#include <QDir>
#include <QFile>
#include <QJsonParseError>
#include <QSaveFile>
#include <QStandardPaths>

namespace kgl::infrastructure {

QString JsonFileStorage::dataDirectory()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation)
        + QStringLiteral("/data");
}

bool JsonFileStorage::ensureDataDirectory(QString* error)
{
    const QString path = dataDirectory();
    if (QDir().mkpath(path)) {
        return true;
    }

    if (error != nullptr) {
        *error = QStringLiteral("无法创建数据目录：%1").arg(path);
    }
    return false;
}

bool JsonFileStorage::read(const QString& filePath, QJsonDocument* document, QString* error)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        if (error != nullptr) {
            *error = file.errorString();
        }
        return false;
    }

    QJsonParseError parseError;
    const QJsonDocument parsed = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        if (error != nullptr) {
            *error = parseError.errorString();
        }
        return false;
    }

    if (document != nullptr) {
        *document = parsed;
    }
    return true;
}

bool JsonFileStorage::write(const QString& filePath, const QJsonDocument& document, QString* error)
{
    if (!ensureDataDirectory(error)) {
        return false;
    }

    QSaveFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        if (error != nullptr) {
            *error = file.errorString();
        }
        return false;
    }

    const QByteArray payload = document.toJson(QJsonDocument::Indented);
    if (file.write(payload) != payload.size()) {
        if (error != nullptr) {
            *error = file.errorString();
        }
        return false;
    }
    if (!file.commit()) {
        if (error != nullptr) {
            *error = file.errorString();
        }
        return false;
    }
    return true;
}

} // namespace kgl::infrastructure
