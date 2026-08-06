#pragma once

#include "domain/repositories/INotesRepository.h"

namespace kgl::infrastructure {

class JsonNotesRepository final : public domain::INotesRepository {
public:
    QString load(QString* error = nullptr) override;
    bool save(const QString& content, QString* error = nullptr) override;

private:
    QString filePath() const;
};

} // namespace kgl::infrastructure
