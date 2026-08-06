#pragma once

#include "domain/entities/OnlineQuestion.h"

#include <QHash>
#include <QNetworkAccessManager>
#include <QObject>
#include <QUrl>

class QNetworkReply;
class QNetworkRequest;

namespace kgl::infrastructure {

class BalaApiClient final : public QObject {
    Q_OBJECT

public:
    explicit BalaApiClient(QObject* parent = nullptr);

    void fetchPapers(int page = 1, int pageSize = 50);
    void fetchPaper(int paperId);

signals:
    void papersFetched(const QVector<domain::PaperSummary>& papers, int total);
    void paperFetched(const domain::PaperDetail& paper);
    void requestFailed(const QString& message);

private:
    QNetworkRequest createRequest(const QUrl& url) const;
    void createGuestSession(int paperId);
    void requestPaper(int paperId, const QString& sessionId);
    QString responseError(QNetworkReply* reply, const QByteArray& payload,
        const QString& fallback) const;

    QUrl baseUrl_ = QUrl(QStringLiteral("https://balagk.com"));
    QNetworkAccessManager network_;
    QHash<int, QString> guestSessionIds_;
};

} // namespace kgl::infrastructure
