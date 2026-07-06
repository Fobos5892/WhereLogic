#pragma once

#include <QByteArray>
#include <QJsonArray>
#include <QMutex>
#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QString>
#include <QVariant>
#include <QVector>

struct GamePresetInfo {
    int id = 0;
    QString name;
    int roundCount = 0;
};

struct RoundInfo {
    int id = 0;
    QString titleKey;
    QString ruleTextKey;
    QString layoutType;
    int timerDuration = 60;
    int stealDuration = 15;
};

struct PuzzleInfo {
    int id = 0;
    int roundId = 0;
    int sortOrder = 0;
    QString correctAnswer;
    QString hintTextKey;
    int points = 100;
    int templateId = 0;
    QString quoteSlotsJson;
    QString correctOrder;
};

struct MaskTemplateInfo {
    int id = 0;
    QString name;
    QString contourPoints;
};

struct TeamInfo {
    int id = 0;
    QString name;
    int score = 0;
};

struct GameStateSnapshot {
    int presetId = 0;
    int roundId = 0;
    int puzzleId = 0;
    int turnTeamId = 0;
    QString stage = QStringLiteral("STAGE_CLOSED_CARDS");
    QString subState = QStringLiteral("MAIN_TURN");
    int roundScoreTeamA = 0;
    int roundScoreTeamB = 0;
    bool activeSession = false;
};

class DatabaseManager : public QObject
{
    Q_OBJECT

public:
    explicit DatabaseManager(QObject *parent = nullptr);
    ~DatabaseManager() override;

    bool initialize();
    QString databasePath() const { return m_databasePath; }

    bool seedCatalogRounds();
    bool seedTestPreset();

    QVector<GamePresetInfo> listPresets() const;
    QVector<RoundInfo> listAllRounds() const;
    QVector<RoundInfo> listRoundsForPreset(int presetId) const;
    QVector<PuzzleInfo> listPuzzlesForRound(int roundId) const;
    QVector<int> presetRoundIds(int presetId) const;
    QVector<TeamInfo> listTeams() const;

    int createPreset(const QString &name);
    bool renamePreset(int presetId, const QString &name);
    bool deletePreset(int presetId);
    bool setPresetRounds(int presetId, const QVector<int> &roundIdsInOrder);

    int createPuzzle(int roundId, const QString &answer, const QString &hintText);
    bool updatePuzzle(int puzzleId,
                       const QString &answer,
                       const QString &hintText,
                       const QString &quoteSlotsJson = {});
    bool deletePuzzle(int puzzleId);
    bool setPuzzleTemplateId(int puzzleId, int templateId);

    bool upsertLocalizationString(const QString &key, const QString &ruText, const QString &enText = {});

    int upsertMaskTemplate(int templateId,
                           const QString &name,
                           const QByteArray &imageData,
                           const QString &contourPoints);
    MaskTemplateInfo maskTemplateById(int templateId) const;
    QByteArray maskTemplateImageData(int templateId) const;
    QString maskTemplateContour(int templateId) const;

    RoundInfo roundById(int roundId) const;
    PuzzleInfo puzzleById(int puzzleId) const;
    QString localizedString(const QString &key, const QString &langCode = QStringLiteral("ru")) const;

    bool saveTeams(const QVector<TeamInfo> &teams);
    bool clearTeams();

    GameStateSnapshot loadGameState() const;
    bool saveGameState(const GameStateSnapshot &state);

    QByteArray puzzleImageData(int puzzleId, int slotIndex) const;
    bool upsertPuzzleImage(int puzzleId, int slotIndex, const QByteArray &imageData);

signals:
    void databaseError(const QString &message);

private:
    bool openDatabase();
    bool createSchema();
    bool executeSchemaStatements();
    bool seedLanguagesAndStrings();
    bool repairStoredTextEncoding();
    bool syncUiDefaultsToDatabase();
    bool insertCatalogRound(int id,
                            const QString &titleKey,
                            const QString &ruleKey,
                            const QString &layoutType,
                            int timerDuration,
                            int stealDuration);
    bool insertSamplePuzzle(int roundId,
                            int sortOrder,
                            const QString &answer,
                            const QString &hintKey,
                            const QString &quoteSlotsJson = {},
                            const QString &correctOrder = {});

    QSqlDatabase database() const;
    bool execQuery(const QString &sql, const QVariantList &bindValues = {}) const;
    QSqlQuery prepareQuery(const QString &sql) const;

    mutable QMutex m_mutex;
    QString m_connectionName;
    QString m_databasePath;
};
