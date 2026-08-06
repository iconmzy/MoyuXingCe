#include "infrastructure/storage/JsonNotesRepository.h"

#include "infrastructure/storage/JsonFileStorage.h"

#include <QFile>
#include <QJsonObject>

namespace kgl::infrastructure {

QString JsonNotesRepository::filePath() const
{
    return JsonFileStorage::dataDirectory() + QStringLiteral("/notes.json");
}

QString JsonNotesRepository::load(QString* error)
{
    if (!QFile::exists(filePath())) {
        return {};
    }

    QJsonDocument document;
    if (!JsonFileStorage::read(filePath(), &document, error) || !document.isObject()) {
        return {};
    }
    return document.object().value(QStringLiteral("content")).toString();
}

bool JsonNotesRepository::save(const QString& content, QString* error)
{
    QJsonObject object;
    object.insert(QStringLiteral("content"), content);
    return JsonFileStorage::write(filePath(), QJsonDocument(object), error);
}

} // namespace kgl::infrastructure
