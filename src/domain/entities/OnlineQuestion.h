#pragma once

#include <QMetaType>
#include <QString>
#include <QStringList>
#include <QVector>

namespace kgl::domain {

struct PaperSummary {
    int id = 0;
    QString title;
    QString category;
    QString region;
    int rating = 0;
    int questionCount = 0;
};

struct OnlineQuestion {
    qint64 id = 0;
    int paperId = 0;
    QString type;
    QString question;
    QString material;
    QStringList options;
    QString answer;
    QString explanation;
    QString knowledgePoint;
    QString subCategory;
    QString difficulty;
    int sortOrder = 0;
};

struct PaperDetail {
    PaperSummary summary;
    QVector<OnlineQuestion> questions;
};

struct AnswerResult {
    bool answered = false;
    bool correct = false;
    QString selectedAnswer;
    QString correctAnswer;
    QString explanation;
};

} // namespace kgl::domain

Q_DECLARE_METATYPE(kgl::domain::PaperSummary)
Q_DECLARE_METATYPE(kgl::domain::OnlineQuestion)
Q_DECLARE_METATYPE(kgl::domain::PaperDetail)
Q_DECLARE_METATYPE(kgl::domain::AnswerResult)
