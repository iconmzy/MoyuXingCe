#pragma once

#include <QString>

namespace kgl::domain {

class INotesRepository {
public:
    virtual ~INotesRepository() = default;
    virtual QString load(QString* error = nullptr) = 0;
    virtual bool save(const QString& content, QString* error = nullptr) = 0;
};

} // namespace kgl::domain
