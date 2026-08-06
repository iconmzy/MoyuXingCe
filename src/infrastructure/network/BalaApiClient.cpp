#include "infrastructure/network/BalaApiClient.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrlQuery>

namespace {

kgl::domain::PaperSummary parsePaperSummary(const QJsonObject& object)
{
    kgl::domain::PaperSummary paper;
    paper.id = object.value(QStringLiteral("id")).toInt();
    paper.title = object.value(QStringLiteral("title")).toString();
    paper.category = object.value(QStringLiteral("category")).toString();
    paper.region = object.value(QStringLiteral("region")).toString();
    paper.rating = object.value(QStringLiteral("rating")).toInt();
    paper.questionCount = object.value(QStringLiteral("question_count")).toInt();
    return paper;
}

kgl::domain::OnlineQuestion parseQuestion(const QJsonObject& object)
{
    kgl::domain::OnlineQuestion question;
    question.id = object.value(QStringLiteral("id")).toInteger();
    question.paperId = object.value(QStringLiteral("paper_id")).toInt();
    question.type = object.value(QStringLiteral("type")).toString();
    question.question = object.value(QStringLiteral("question")).toString();
    question.material = object.value(QStringLiteral("material")).toString();
    question.answer = object.value(QStringLiteral("answer")).toString().trimmed().toUpper();
    question.explanation = object.value(QStringLiteral("explanation")).toString();
    question.knowledgePoint = object.value(QStringLiteral("knowledge_point")).toString();
    question.subCategory = object.value(QStringLiteral("sub_category")).toString();
    question.difficulty = object.value(QStringLiteral("difficulty")).toString();
    question.sortOrder = object.value(QStringLiteral("sort_order")).toInt();

    const QJsonArray options = object.value(QStringLiteral("options")).toArray();
    for (const QJsonValue& value : options) {
        question.options.append(value.toString());
    }
    return question;
}

} // namespace

namespace kgl::infrastructure {

BalaApiClient::BalaApiClient(QObject* parent)
    : QObject(parent)
{
}

QNetworkRequest BalaApiClient::createRequest(const QUrl& url) const
{
    QNetworkRequest request(url);
    request.setRawHeader("User-Agent", "KeepGongLearning/0.3");
    request.setRawHeader("Accept", "application/json");
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
        QNetworkRequest::NoLessSafeRedirectPolicy);
    return request;
}

void BalaApiClient::fetchPapers(int page, int pageSize)
{
    QUrl url = baseUrl_.resolved(QUrl(QStringLiteral("/api/papers")));
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("category"), QStringLiteral("行测"));
    query.addQueryItem(QStringLiteral("page"), QString::number(qMax(1, page)));
    query.addQueryItem(QStringLiteral("pageSize"), QString::number(qBound(1, pageSize, 100)));
    url.setQuery(query);

    QNetworkReply* reply = network_.get(createRequest(url));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        const QByteArray payload = reply->readAll();
        if (reply->error() != QNetworkReply::NoError) {
            emit requestFailed(responseError(reply, payload, QStringLiteral("试卷列表加载失败")));
            reply->deleteLater();
            return;
        }

        QJsonParseError parseError;
        const QJsonDocument document = QJsonDocument::fromJson(payload, &parseError);
        if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
            emit requestFailed(QStringLiteral("试卷列表响应格式无效"));
            reply->deleteLater();
            return;
        }

        QVector<domain::PaperSummary> papers;
        const QJsonObject root = document.object();
        for (const QJsonValue& value : root.value(QStringLiteral("papers")).toArray()) {
            if (!value.isObject()) {
                continue;
            }
            const domain::PaperSummary paper = parsePaperSummary(value.toObject());
            if (paper.id > 0 && !paper.title.isEmpty()) {
                papers.append(paper);
            }
        }
        emit papersFetched(papers, root.value(QStringLiteral("total")).toInt(papers.size()));
        reply->deleteLater();
    });
}

void BalaApiClient::fetchPaper(int paperId)
{
    if (paperId <= 0) {
        emit requestFailed(QStringLiteral("试卷编号无效"));
        return;
    }

    const QString existingSession = guestSessionIds_.value(paperId);
    if (!existingSession.isEmpty()) {
        requestPaper(paperId, existingSession);
        return;
    }
    createGuestSession(paperId);
}

void BalaApiClient::createGuestSession(int paperId)
{
    const QUrl url = baseUrl_.resolved(QUrl(QStringLiteral("/api/practice-access/guest-sessions")));
    QNetworkRequest request = createRequest(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));

    QJsonObject target;
    target.insert(QStringLiteral("paperId"), paperId);
    QJsonObject body;
    body.insert(QStringLiteral("mode"), QStringLiteral("xingce_paper"));
    body.insert(QStringLiteral("target"), target);

    QNetworkReply* reply = network_.post(request, QJsonDocument(body).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, this, [this, reply, paperId]() {
        const QByteArray payload = reply->readAll();
        if (reply->error() != QNetworkReply::NoError) {
            emit requestFailed(responseError(reply, payload, QStringLiteral("游客练习会话创建失败")));
            reply->deleteLater();
            return;
        }

        const QJsonDocument document = QJsonDocument::fromJson(payload);
        const QString sessionId = document.object().value(QStringLiteral("sessionId")).toString();
        if (sessionId.isEmpty()) {
            emit requestFailed(QStringLiteral("网站未返回游客练习会话"));
            reply->deleteLater();
            return;
        }

        guestSessionIds_.insert(paperId, sessionId);
        reply->deleteLater();
        requestPaper(paperId, sessionId);
    });
}

void BalaApiClient::requestPaper(int paperId, const QString& sessionId)
{
    const QUrl url = baseUrl_.resolved(QUrl(QStringLiteral("/api/papers/%1").arg(paperId)));
    QNetworkRequest request = createRequest(url);
    request.setRawHeader("X-Guest-Practice-Session", sessionId.toUtf8());

    QNetworkReply* reply = network_.get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply, paperId]() {
        const QByteArray payload = reply->readAll();
        if (reply->error() != QNetworkReply::NoError) {
            guestSessionIds_.remove(paperId);
            emit requestFailed(responseError(reply, payload, QStringLiteral("试卷题目加载失败")));
            reply->deleteLater();
            return;
        }

        QJsonParseError parseError;
        const QJsonDocument document = QJsonDocument::fromJson(payload, &parseError);
        if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
            emit requestFailed(QStringLiteral("试卷响应格式无效"));
            reply->deleteLater();
            return;
        }

        const QJsonObject root = document.object();
        domain::PaperDetail paper;
        paper.summary = parsePaperSummary(root);
        for (const QJsonValue& value : root.value(QStringLiteral("questions")).toArray()) {
            if (!value.isObject()) {
                continue;
            }
            domain::OnlineQuestion question = parseQuestion(value.toObject());
            if (question.id > 0 && !question.question.isEmpty() && !question.options.isEmpty()) {
                paper.questions.append(question);
            }
        }

        if (paper.questions.isEmpty()) {
            emit requestFailed(QStringLiteral("该试卷没有可展示的选择题"));
        } else {
            emit paperFetched(paper);
        }
        reply->deleteLater();
    });
}

QString BalaApiClient::responseError(QNetworkReply* reply, const QByteArray& payload,
    const QString& fallback) const
{
    const QJsonDocument document = QJsonDocument::fromJson(payload);
    if (document.isObject()) {
        const QJsonObject object = document.object();
        const QString message = object.value(QStringLiteral("message")).toString();
        const QString code = object.value(QStringLiteral("code")).toString();
        if (!message.isEmpty()) {
            return code.isEmpty() ? message : QStringLiteral("%1（%2）").arg(message, code);
        }
    }

    const QString networkMessage = reply->errorString();
    return networkMessage.isEmpty()
        ? fallback
        : QStringLiteral("%1：%2").arg(fallback, networkMessage);
}

} // namespace kgl::infrastructure
