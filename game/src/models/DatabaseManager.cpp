#include "DatabaseManager.h"

#include "../core/GameConstants.h"
#include "../core/UiStringDefaults.h"

#include <QDir>
#include <QJsonDocument>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QUuid>

namespace {

constexpr QLatin1String kTestPresetName("Тестовая игра");

} // namespace

DatabaseManager::DatabaseManager(QObject *parent)
    : QObject(parent)
    , m_connectionName(QStringLiteral("wherelogic_%1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces)))
{
}

DatabaseManager::~DatabaseManager()
{
    QMutexLocker locker(&m_mutex);
    if (QSqlDatabase::contains(m_connectionName)) {
        {
            QSqlDatabase db = QSqlDatabase::database(m_connectionName);
            if (db.isOpen()) {
                db.close();
            }
        }
        QSqlDatabase::removeDatabase(m_connectionName);
    }
}

bool DatabaseManager::initialize()
{
    if (!UiStringDefaults::instance().loadFromResource()) {
        emit databaseError(QStringLiteral("Не удалось загрузить ui_defaults_ru.json — используются только строки из БД"));
    }

    {
        QMutexLocker locker(&m_mutex);

        const QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        if (dataDir.isEmpty()) {
            emit databaseError(QStringLiteral("AppDataLocation is unavailable"));
            return false;
        }

        QDir().mkpath(dataDir);
        m_databasePath = dataDir + QStringLiteral("/wherelogic.db");

        if (!openDatabase()) {
            return false;
        }
        if (!createSchema()) {
            return false;
        }
        if (!seedLanguagesAndStrings()) {
            return false;
        }
    }

    QSqlQuery countQuery(database());
    if (!countQuery.exec(QStringLiteral("SELECT COUNT(*) FROM rounds"))) {
        emit databaseError(countQuery.lastError().text());
        return false;
    }
    if (countQuery.next() && countQuery.value(0).toInt() == 0) {
        if (!seedCatalogRounds()) {
            return false;
        }
    }

    QSqlQuery presetCountQuery(database());
    presetCountQuery.prepare(QStringLiteral("SELECT COUNT(*) FROM game_presets WHERE preset_name = ?"));
    presetCountQuery.addBindValue(QString(kTestPresetName));
    if (!presetCountQuery.exec()) {
        emit databaseError(presetCountQuery.lastError().text());
        return false;
    }

    bool hasTestPreset = false;
    if (presetCountQuery.next()) {
        hasTestPreset = presetCountQuery.value(0).toInt() > 0;
    }
    if (!hasTestPreset && !seedTestPreset()) {
        return false;
    }

    return true;
}

bool DatabaseManager::openDatabase()
{
    QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_connectionName);
    db.setDatabaseName(m_databasePath);
    if (!db.open()) {
        emit databaseError(db.lastError().text());
        return false;
    }

    QSqlQuery pragma(db);
    if (!pragma.exec(QStringLiteral("PRAGMA foreign_keys = ON"))) {
        emit databaseError(pragma.lastError().text());
        return false;
    }
    return true;
}

bool DatabaseManager::createSchema()
{
    return executeSchemaStatements();
}

bool DatabaseManager::executeSchemaStatements()
{
    const QStringList statements = {
        QStringLiteral(
            "CREATE TABLE IF NOT EXISTS languages ("
            "lang_code TEXT PRIMARY KEY,"
            "lang_name TEXT NOT NULL)"),
        QStringLiteral(
            "CREATE TABLE IF NOT EXISTS localization_strings ("
            "string_key TEXT NOT NULL,"
            "lang_code TEXT NOT NULL,"
            "translated_text TEXT NOT NULL,"
            "PRIMARY KEY (string_key, lang_code),"
            "FOREIGN KEY(lang_code) REFERENCES languages(lang_code) ON DELETE CASCADE)"),
        QStringLiteral(
            "CREATE TABLE IF NOT EXISTS game_presets ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "preset_name TEXT NOT NULL)"),
        QStringLiteral(
            "CREATE TABLE IF NOT EXISTS mask_templates ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "template_name TEXT NOT NULL,"
            "image_data BLOB NOT NULL,"
            "contour_points TEXT NOT NULL)"),
        QStringLiteral(
            "CREATE TABLE IF NOT EXISTS rounds ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "timer_duration INTEGER NOT NULL DEFAULT 60,"
            "steal_duration INTEGER NOT NULL DEFAULT 15,"
            "layout_type TEXT NOT NULL DEFAULT 'STANDARD',"
            "rule_text_key TEXT NOT NULL,"
            "title_key TEXT NOT NULL)"),
        QStringLiteral(
            "CREATE TABLE IF NOT EXISTS preset_rounds ("
            "preset_id INTEGER,"
            "round_id INTEGER,"
            "sort_order INTEGER NOT NULL,"
            "is_active INTEGER DEFAULT 1,"
            "PRIMARY KEY (preset_id, round_id),"
            "FOREIGN KEY(preset_id) REFERENCES game_presets(id) ON DELETE CASCADE,"
            "FOREIGN KEY(round_id) REFERENCES rounds(id) ON DELETE CASCADE)"),
        QStringLiteral(
            "CREATE TABLE IF NOT EXISTS puzzles ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "round_id INTEGER NOT NULL,"
            "sort_order INTEGER NOT NULL,"
            "correct_answer TEXT NOT NULL,"
            "hint_text_key TEXT,"
            "points INTEGER DEFAULT 100,"
            "template_id INTEGER REFERENCES mask_templates(id),"
            "puzzle_quote_slots TEXT,"
            "correct_order TEXT,"
            "FOREIGN KEY(round_id) REFERENCES rounds(id) ON DELETE CASCADE)"),
        QStringLiteral(
            "CREATE TABLE IF NOT EXISTS puzzle_images ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "puzzle_id INTEGER NOT NULL,"
            "slot_index INTEGER NOT NULL,"
            "image_data BLOB NOT NULL,"
            "FOREIGN KEY(puzzle_id) REFERENCES puzzles(id) ON DELETE CASCADE)"),
        QStringLiteral(
            "CREATE TABLE IF NOT EXISTS current_teams ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "team_name TEXT NOT NULL,"
            "score INTEGER DEFAULT 0)"),
        QStringLiteral(
            "CREATE TABLE IF NOT EXISTS game_state ("
            "id INTEGER PRIMARY KEY CHECK (id = 1),"
            "current_preset_id INTEGER,"
            "current_round_id INTEGER,"
            "current_puzzle_id INTEGER,"
            "current_turn_team_id INTEGER,"
            "current_stage TEXT NOT NULL DEFAULT 'STAGE_CLOSED_CARDS',"
            "current_sub_state TEXT NOT NULL DEFAULT 'MAIN_TURN',"
            "round_score_team_A INTEGER DEFAULT 0,"
            "round_score_team_B INTEGER DEFAULT 0,"
            "is_active_session INTEGER DEFAULT 0,"
            "FOREIGN KEY(current_preset_id) REFERENCES game_presets(id),"
            "FOREIGN KEY(current_round_id) REFERENCES rounds(id),"
            "FOREIGN KEY(current_puzzle_id) REFERENCES puzzles(id))"),
    };

    QSqlDatabase db = database();
    for (const QString &sql : statements) {
        QSqlQuery query(db);
        if (!query.exec(sql)) {
            emit databaseError(query.lastError().text());
            return false;
        }
    }

    QSqlQuery bootstrap(database());
    if (!bootstrap.exec(QStringLiteral("INSERT OR IGNORE INTO game_state (id) VALUES (1)"))) {
        emit databaseError(bootstrap.lastError().text());
        return false;
    }

    return true;
}

bool DatabaseManager::seedLanguagesAndStrings()
{
    const struct StringSeed {
        const char *key;
        const char *ru;
        const char *en;
    } seeds[] = {
        {"round.1.title", "Найди общее", "Find the Connection"},
        {"round.1.rule", "Найдите общую логику между картинками.", "Find what connects the images."},
        {"round.2.title", "Мама против Google", "Mom vs Google"},
        {"round.2.rule", "Угадайте слово по ассоциациям поколений.", "Guess the word from generational clues."},
        {"round.3.title", "Формула всего", "The Formula of Everything"},
        {"round.3.rule", "Сложите два образа в один ответ.", "Combine two images into one answer."},
        {"round.4.title", "Кино-Логика", "Pop-Culture Logic"},
        {"round.4.rule", "Назовите фильм или сериал по ребусу.", "Name the movie or show from the rebus."},
        {"round.5.title", "Чего-то не хватает", "The Missing Piece"},
        {"round.5.rule", "Угадайте скрытый объект под маской.", "Guess the object hidden under the mask."},
        {"round.6.title", "Два в одном", "Neural Face-Mix"},
        {"round.6.rule", "Назовите обоих людей на гибридном фото.", "Name both people in the hybrid photo."},
        {"round.7.title", "Чей туфля?", "Family Artifacts"},
        {"round.7.rule", "Что объединяет эти семейные артефакты?", "What connects these family artifacts?"},
        {"round.8.title", "Назад в будущее", "Chronology of Life"},
        {"round.8.rule", "Расположите фото в хронологическом порядке.", "Arrange photos in chronological order."},
        {"round.9.title", "Устами младенца", "Abstract Child Logic"},
        {"round.9.rule", "Расшифруйте детские цитаты.", "Decode the child quotes."},
        {"round.10.title", "Финал: Блиц-связь", "The Grand Blitz"},
        {"round.10.rule", "Блиц: найдите связь за секунды.", "Blitz: find the connection in seconds."},
        {"puzzle.hint.paris", "Любимая страна мамы", "Mom's favorite country"},
        {"puzzle.hint.cringe", "Молодёжный сленг", "Youth slang"},
        {"puzzle.hint.pie", "Фирменная выпечка", "Signature baking"},
        {"puzzle.hint.carmelita", "Любимый сериал", "Favorite TV show"},
        {"puzzle.hint.bouquet", "Семейный праздник", "Family celebration"},
        {"puzzle.hint.fen", "Бытовой прибор", "Household appliance"},
        {"puzzle.hint.blitz", "Быстрая связь", "Quick connection"},
    };

    if (!execQuery(QStringLiteral("INSERT OR IGNORE INTO languages (lang_code, lang_name) VALUES (?, ?)"),
                   {QStringLiteral("ru"), QStringLiteral("Русский")})) {
        return false;
    }
    if (!execQuery(QStringLiteral("INSERT OR IGNORE INTO languages (lang_code, lang_name) VALUES (?, ?)"),
                   {QStringLiteral("en"), QStringLiteral("English")})) {
        return false;
    }

    for (const StringSeed &seed : seeds) {
        if (!execQuery(QStringLiteral(
                           "INSERT OR IGNORE INTO localization_strings (string_key, lang_code, translated_text) "
                           "VALUES (?, ?, ?)"),
                       {QString(seed.key), QStringLiteral("ru"), QString(seed.ru)})) {
            return false;
        }
        if (!execQuery(QStringLiteral(
                           "INSERT OR IGNORE INTO localization_strings (string_key, lang_code, translated_text) "
                           "VALUES (?, ?, ?)"),
                       {QString(seed.key), QStringLiteral("en"), QString(seed.en)})) {
            return false;
        }
    }

    return true;
}

bool DatabaseManager::seedCatalogRounds()
{
    QMutexLocker locker(&m_mutex);
    struct RoundSeed {
        int id;
        const char *titleKey;
        const char *ruleKey;
        const char *layoutType;
        int timer;
        int steal;
    };

    const RoundSeed catalog[] = {
        {1, "round.1.title", "round.1.rule", "STANDARD", 60, 15},
        {2, "round.2.title", "round.2.rule", "STANDARD", 60, 15},
        {3, "round.3.title", "round.3.rule", "EQUATION", 90, 22},
        {4, "round.4.title", "round.4.rule", "STANDARD", 90, 22},
        {5, "round.5.title", "round.5.rule", "FULL_MASK", 120, 30},
        {6, "round.6.title", "round.6.rule", "SINGLE_HYBRID", 90, 22},
        {7, "round.7.title", "round.7.rule", "STANDARD", 90, 22},
        {8, "round.8.title", "round.8.rule", "CHRONOLOGY", 120, 30},
        {9, "round.9.title", "round.9.rule", "TEXT_ONLY", 90, 22},
        {10, "round.10.title", "round.10.rule", "BLITZ_STANDARD", 15, 15},
    };

    for (const RoundSeed &round : catalog) {
        if (!insertCatalogRound(round.id,
                                QString(round.titleKey),
                                QString(round.ruleKey),
                                QString(round.layoutType),
                                round.timer,
                                round.steal)) {
            return false;
        }
    }

    return true;
}

bool DatabaseManager::insertCatalogRound(int id,
                                         const QString &titleKey,
                                         const QString &ruleKey,
                                         const QString &layoutType,
                                         int timerDuration,
                                         int stealDuration)
{
    QSqlQuery query(database());
    query.prepare(QStringLiteral(
        "INSERT OR IGNORE INTO rounds (id, timer_duration, steal_duration, layout_type, rule_text_key, title_key) "
        "VALUES (?, ?, ?, ?, ?, ?)"));
    query.addBindValue(id);
    query.addBindValue(timerDuration);
    query.addBindValue(stealDuration);
    query.addBindValue(layoutType);
    query.addBindValue(ruleKey);
    query.addBindValue(titleKey);
    if (!query.exec()) {
        emit databaseError(query.lastError().text());
        return false;
    }
    return true;
}

bool DatabaseManager::seedTestPreset()
{
    QMutexLocker locker(&m_mutex);
    QSqlQuery presetQuery(database());
    presetQuery.prepare(QStringLiteral("INSERT INTO game_presets (preset_name) VALUES (?)"));
    presetQuery.addBindValue(QString(kTestPresetName));
    if (!presetQuery.exec()) {
        emit databaseError(presetQuery.lastError().text());
        return false;
    }

    const int presetId = presetQuery.lastInsertId().toInt();
    const int roundIds[] = {1, 5, 9};
    for (int i = 0; i < 3; ++i) {
        QSqlQuery linkQuery(database());
        linkQuery.prepare(QStringLiteral(
            "INSERT INTO preset_rounds (preset_id, round_id, sort_order, is_active) VALUES (?, ?, ?, 1)"));
        linkQuery.addBindValue(presetId);
        linkQuery.addBindValue(roundIds[i]);
        linkQuery.addBindValue(i + 1);
        if (!linkQuery.exec()) {
            emit databaseError(linkQuery.lastError().text());
            return false;
        }
    }

    if (!insertSamplePuzzle(1, 1, QStringLiteral("Париж"), QStringLiteral("puzzle.hint.paris"))) {
        return false;
    }
    if (!insertSamplePuzzle(5, 1, QStringLiteral("Свадебный букет"), QStringLiteral("puzzle.hint.bouquet"))) {
        return false;
    }

    const QJsonArray quotes = {
        QStringLiteral("Оно жужжит и пугает кота"),
        QStringLiteral("Мама пользуется им перед дачей"),
        QStringLiteral("Им папа сушит носки, когда мама не видит"),
    };
    if (!insertSamplePuzzle(9,
                            1,
                            QStringLiteral("Фен"),
                            QStringLiteral("puzzle.hint.fen"),
                            QString::fromUtf8(QJsonDocument(quotes).toJson(QJsonDocument::Compact)))) {
        return false;
    }

    return true;
}

bool DatabaseManager::insertSamplePuzzle(int roundId,
                                         int sortOrder,
                                         const QString &answer,
                                         const QString &hintKey,
                                         const QString &quoteSlotsJson,
                                         const QString &correctOrder)
{
    QSqlQuery query(database());
    query.prepare(QStringLiteral(
        "INSERT INTO puzzles (round_id, sort_order, correct_answer, hint_text_key, points, puzzle_quote_slots, "
        "correct_order) VALUES (?, ?, ?, ?, 100, ?, ?)"));
    query.addBindValue(roundId);
    query.addBindValue(sortOrder);
    query.addBindValue(answer);
    query.addBindValue(hintKey);
    query.addBindValue(quoteSlotsJson.isEmpty() ? QVariant() : quoteSlotsJson);
    query.addBindValue(correctOrder.isEmpty() ? QVariant() : correctOrder);
    if (!query.exec()) {
        emit databaseError(query.lastError().text());
        return false;
    }
    return true;
}

QVector<GamePresetInfo> DatabaseManager::listPresets() const
{
    QMutexLocker locker(&m_mutex);
    QVector<GamePresetInfo> presets;

    QSqlQuery query(database());
    if (!query.exec(QStringLiteral(
            "SELECT gp.id, gp.preset_name, COUNT(pr.round_id) "
            "FROM game_presets gp "
            "LEFT JOIN preset_rounds pr ON pr.preset_id = gp.id AND pr.is_active = 1 "
            "GROUP BY gp.id, gp.preset_name "
            "ORDER BY gp.id"))) {
        emit const_cast<DatabaseManager *>(this)->databaseError(query.lastError().text());
        return presets;
    }

    while (query.next()) {
        GamePresetInfo info;
        info.id = query.value(0).toInt();
        info.name = query.value(1).toString();
        info.roundCount = query.value(2).toInt();
        presets.append(info);
    }
    return presets;
}

QVector<RoundInfo> DatabaseManager::listAllRounds() const
{
    QMutexLocker locker(&m_mutex);
    QVector<RoundInfo> rounds;

    QSqlQuery query(database());
    if (!query.exec(QStringLiteral(
            "SELECT id, title_key, rule_text_key, layout_type, timer_duration, steal_duration "
            "FROM rounds ORDER BY id"))) {
        emit const_cast<DatabaseManager *>(this)->databaseError(query.lastError().text());
        return rounds;
    }

    while (query.next()) {
        RoundInfo info;
        info.id = query.value(0).toInt();
        info.titleKey = query.value(1).toString();
        info.ruleTextKey = query.value(2).toString();
        info.layoutType = query.value(3).toString();
        info.timerDuration = query.value(4).toInt();
        info.stealDuration = query.value(5).toInt();
        rounds.append(info);
    }
    return rounds;
}

QVector<int> DatabaseManager::presetRoundIds(int presetId) const
{
    QMutexLocker locker(&m_mutex);
    QVector<int> roundIds;

    QSqlQuery query(database());
    query.prepare(QStringLiteral(
        "SELECT round_id FROM preset_rounds WHERE preset_id = ? AND is_active = 1 ORDER BY sort_order"));
    query.addBindValue(presetId);
    if (!query.exec()) {
        emit const_cast<DatabaseManager *>(this)->databaseError(query.lastError().text());
        return roundIds;
    }

    while (query.next()) {
        roundIds.append(query.value(0).toInt());
    }
    return roundIds;
}

int DatabaseManager::createPreset(const QString &name)
{
    QMutexLocker locker(&m_mutex);

    QSqlQuery query(database());
    query.prepare(QStringLiteral("INSERT INTO game_presets (preset_name) VALUES (?)"));
    query.addBindValue(name.trimmed());
    if (!query.exec()) {
        emit databaseError(query.lastError().text());
        return 0;
    }
    return query.lastInsertId().toInt();
}

bool DatabaseManager::renamePreset(int presetId, const QString &name)
{
    QMutexLocker locker(&m_mutex);

    QSqlQuery query(database());
    query.prepare(QStringLiteral("UPDATE game_presets SET preset_name = ? WHERE id = ?"));
    query.addBindValue(name.trimmed());
    query.addBindValue(presetId);
    if (!query.exec()) {
        emit databaseError(query.lastError().text());
        return false;
    }
    return query.numRowsAffected() > 0;
}

bool DatabaseManager::deletePreset(int presetId)
{
    QMutexLocker locker(&m_mutex);

    QSqlQuery query(database());
    query.prepare(QStringLiteral("DELETE FROM game_presets WHERE id = ?"));
    query.addBindValue(presetId);
    if (!query.exec()) {
        emit databaseError(query.lastError().text());
        return false;
    }
    return query.numRowsAffected() > 0;
}

bool DatabaseManager::setPresetRounds(int presetId, const QVector<int> &roundIdsInOrder)
{
    QMutexLocker locker(&m_mutex);
    QSqlDatabase db = database();
    if (!db.transaction()) {
        emit databaseError(db.lastError().text());
        return false;
    }

    QSqlQuery deleteQuery(db);
    deleteQuery.prepare(QStringLiteral("DELETE FROM preset_rounds WHERE preset_id = ?"));
    deleteQuery.addBindValue(presetId);
    if (!deleteQuery.exec()) {
        emit databaseError(deleteQuery.lastError().text());
        db.rollback();
        return false;
    }

    for (int i = 0; i < roundIdsInOrder.size(); ++i) {
        QSqlQuery insertQuery(db);
        insertQuery.prepare(QStringLiteral(
            "INSERT INTO preset_rounds (preset_id, round_id, sort_order, is_active) VALUES (?, ?, ?, 1)"));
        insertQuery.addBindValue(presetId);
        insertQuery.addBindValue(roundIdsInOrder.at(i));
        insertQuery.addBindValue(i + 1);
        if (!insertQuery.exec()) {
            emit databaseError(insertQuery.lastError().text());
            db.rollback();
            return false;
        }
    }

    if (!db.commit()) {
        emit databaseError(db.lastError().text());
        return false;
    }
    return true;
}

bool DatabaseManager::upsertLocalizationString(const QString &key, const QString &ruText, const QString &enText)
{
    QMutexLocker locker(&m_mutex);

    const auto upsert = [this, &key](const QString &lang, const QString &text) {
        QSqlQuery query(database());
        query.prepare(QStringLiteral(
            "INSERT INTO localization_strings (string_key, lang_code, translated_text) VALUES (?, ?, ?) "
            "ON CONFLICT(string_key, lang_code) DO UPDATE SET translated_text = excluded.translated_text"));
        query.addBindValue(key);
        query.addBindValue(lang);
        query.addBindValue(text);
        if (!query.exec()) {
            emit databaseError(query.lastError().text());
            return false;
        }
        return true;
    };

    if (!upsert(QStringLiteral("ru"), ruText)) {
        return false;
    }
    if (!enText.isEmpty()) {
        return upsert(QStringLiteral("en"), enText);
    }
    return upsert(QStringLiteral("en"), ruText);
}

int DatabaseManager::createPuzzle(int roundId, const QString &answer, const QString &hintText)
{
    QMutexLocker locker(&m_mutex);
    QSqlDatabase db = database();
    if (!db.transaction()) {
        emit databaseError(db.lastError().text());
        return 0;
    }

    QSqlQuery orderQuery(db);
    orderQuery.prepare(QStringLiteral("SELECT COALESCE(MAX(sort_order), 0) + 1 FROM puzzles WHERE round_id = ?"));
    orderQuery.addBindValue(roundId);
    if (!orderQuery.exec() || !orderQuery.next()) {
        emit databaseError(orderQuery.lastError().text());
        db.rollback();
        return 0;
    }
    const int sortOrder = orderQuery.value(0).toInt();

    QSqlQuery insertQuery(db);
    insertQuery.prepare(QStringLiteral(
        "INSERT INTO puzzles (round_id, sort_order, correct_answer, hint_text_key, points) "
        "VALUES (?, ?, ?, '', 100)"));
    insertQuery.addBindValue(roundId);
    insertQuery.addBindValue(sortOrder);
    insertQuery.addBindValue(answer.trimmed());
    if (!insertQuery.exec()) {
        emit databaseError(insertQuery.lastError().text());
        db.rollback();
        return 0;
    }

    const int puzzleId = insertQuery.lastInsertId().toInt();
    const QString hintKey = QStringLiteral("puzzle.hint.p%1").arg(puzzleId);

    QSqlQuery hintKeyQuery(db);
    hintKeyQuery.prepare(QStringLiteral("UPDATE puzzles SET hint_text_key = ? WHERE id = ?"));
    hintKeyQuery.addBindValue(hintKey);
    hintKeyQuery.addBindValue(puzzleId);
    if (!hintKeyQuery.exec()) {
        emit databaseError(hintKeyQuery.lastError().text());
        db.rollback();
        return 0;
    }

    QSqlQuery locRu(db);
    locRu.prepare(QStringLiteral(
        "INSERT INTO localization_strings (string_key, lang_code, translated_text) VALUES (?, ?, ?) "
        "ON CONFLICT(string_key, lang_code) DO UPDATE SET translated_text = excluded.translated_text"));
    locRu.addBindValue(hintKey);
    locRu.addBindValue(QStringLiteral("ru"));
    locRu.addBindValue(hintText.trimmed());
    if (!locRu.exec()) {
        emit databaseError(locRu.lastError().text());
        db.rollback();
        return 0;
    }

    QSqlQuery locEn(db);
    locEn.prepare(QStringLiteral(
        "INSERT INTO localization_strings (string_key, lang_code, translated_text) VALUES (?, ?, ?) "
        "ON CONFLICT(string_key, lang_code) DO UPDATE SET translated_text = excluded.translated_text"));
    locEn.addBindValue(hintKey);
    locEn.addBindValue(QStringLiteral("en"));
    locEn.addBindValue(hintText.trimmed());
    if (!locEn.exec()) {
        emit databaseError(locEn.lastError().text());
        db.rollback();
        return 0;
    }

    if (!db.commit()) {
        emit databaseError(db.lastError().text());
        return 0;
    }
    return puzzleId;
}

bool DatabaseManager::updatePuzzle(int puzzleId,
                                   const QString &answer,
                                   const QString &hintText,
                                   const QString &quoteSlotsJson)
{
    QMutexLocker locker(&m_mutex);

    QSqlQuery existingQuery(database());
    existingQuery.prepare(QStringLiteral("SELECT hint_text_key FROM puzzles WHERE id = ?"));
    existingQuery.addBindValue(puzzleId);
    if (!existingQuery.exec() || !existingQuery.next()) {
        return false;
    }

    QString hintKey = existingQuery.value(0).toString();
    if (hintKey.isEmpty()) {
        hintKey = QStringLiteral("puzzle.hint.p%1").arg(puzzleId);
    }

    QSqlQuery query(database());
    query.prepare(QStringLiteral(
        "UPDATE puzzles SET correct_answer = ?, hint_text_key = ?, puzzle_quote_slots = ? WHERE id = ?"));
    query.addBindValue(answer.trimmed());
    query.addBindValue(hintKey);
    query.addBindValue(quoteSlotsJson.isEmpty() ? QVariant() : quoteSlotsJson);
    query.addBindValue(puzzleId);
    if (!query.exec()) {
        emit databaseError(query.lastError().text());
        return false;
    }

    const auto upsert = [this, &hintKey](const QString &lang, const QString &text) {
        QSqlQuery locQuery(database());
        locQuery.prepare(QStringLiteral(
            "INSERT INTO localization_strings (string_key, lang_code, translated_text) VALUES (?, ?, ?) "
            "ON CONFLICT(string_key, lang_code) DO UPDATE SET translated_text = excluded.translated_text"));
        locQuery.addBindValue(hintKey);
        locQuery.addBindValue(lang);
        locQuery.addBindValue(text);
        if (!locQuery.exec()) {
            emit databaseError(locQuery.lastError().text());
            return false;
        }
        return true;
    };

    return upsert(QStringLiteral("ru"), hintText.trimmed())
        && upsert(QStringLiteral("en"), hintText.trimmed());
}

bool DatabaseManager::deletePuzzle(int puzzleId)
{
    QMutexLocker locker(&m_mutex);

    QSqlQuery query(database());
    query.prepare(QStringLiteral("DELETE FROM puzzles WHERE id = ?"));
    query.addBindValue(puzzleId);
    if (!query.exec()) {
        emit databaseError(query.lastError().text());
        return false;
    }
    return query.numRowsAffected() > 0;
}

bool DatabaseManager::setPuzzleTemplateId(int puzzleId, int templateId)
{
    QMutexLocker locker(&m_mutex);

    QSqlQuery query(database());
    query.prepare(QStringLiteral("UPDATE puzzles SET template_id = ? WHERE id = ?"));
    query.addBindValue(templateId > 0 ? templateId : QVariant());
    query.addBindValue(puzzleId);
    if (!query.exec()) {
        emit databaseError(query.lastError().text());
        return false;
    }
    return true;
}

int DatabaseManager::upsertMaskTemplate(int templateId,
                                        const QString &name,
                                        const QByteArray &imageData,
                                        const QString &contourPoints)
{
    QMutexLocker locker(&m_mutex);

    if (templateId > 0) {
        QSqlQuery query(database());
        query.prepare(QStringLiteral(
            "UPDATE mask_templates SET template_name = ?, image_data = ?, contour_points = ? WHERE id = ?"));
        query.addBindValue(name);
        query.addBindValue(imageData);
        query.addBindValue(contourPoints);
        query.addBindValue(templateId);
        if (!query.exec()) {
            emit databaseError(query.lastError().text());
            return 0;
        }
        return templateId;
    }

    QSqlQuery query(database());
    query.prepare(QStringLiteral(
        "INSERT INTO mask_templates (template_name, image_data, contour_points) VALUES (?, ?, ?)"));
    query.addBindValue(name);
    query.addBindValue(imageData);
    query.addBindValue(contourPoints);
    if (!query.exec()) {
        emit databaseError(query.lastError().text());
        return 0;
    }
    return query.lastInsertId().toInt();
}

MaskTemplateInfo DatabaseManager::maskTemplateById(int templateId) const
{
    QMutexLocker locker(&m_mutex);
    MaskTemplateInfo info;

    QSqlQuery query(database());
    query.prepare(QStringLiteral("SELECT id, template_name, contour_points FROM mask_templates WHERE id = ?"));
    query.addBindValue(templateId);
    if (!query.exec() || !query.next()) {
        return info;
    }

    info.id = query.value(0).toInt();
    info.name = query.value(1).toString();
    info.contourPoints = query.value(2).toString();
    return info;
}

QByteArray DatabaseManager::maskTemplateImageData(int templateId) const
{
    QMutexLocker locker(&m_mutex);

    QSqlQuery query(database());
    query.prepare(QStringLiteral("SELECT image_data FROM mask_templates WHERE id = ?"));
    query.addBindValue(templateId);
    if (!query.exec() || !query.next()) {
        return {};
    }
    return query.value(0).toByteArray();
}

QString DatabaseManager::maskTemplateContour(int templateId) const
{
    return maskTemplateById(templateId).contourPoints;
}

QVector<RoundInfo> DatabaseManager::listRoundsForPreset(int presetId) const
{
    QMutexLocker locker(&m_mutex);
    QVector<RoundInfo> rounds;

    QSqlQuery query(database());
    query.prepare(QStringLiteral(
        "SELECT r.id, r.title_key, r.rule_text_key, r.layout_type, r.timer_duration, r.steal_duration "
        "FROM preset_rounds pr "
        "JOIN rounds r ON r.id = pr.round_id "
        "WHERE pr.preset_id = ? AND pr.is_active = 1 "
        "ORDER BY pr.sort_order"));
    query.addBindValue(presetId);
    if (!query.exec()) {
        emit const_cast<DatabaseManager *>(this)->databaseError(query.lastError().text());
        return rounds;
    }

    while (query.next()) {
        RoundInfo info;
        info.id = query.value(0).toInt();
        info.titleKey = query.value(1).toString();
        info.ruleTextKey = query.value(2).toString();
        info.layoutType = query.value(3).toString();
        info.timerDuration = query.value(4).toInt();
        info.stealDuration = query.value(5).toInt();
        rounds.append(info);
    }
    return rounds;
}

QVector<PuzzleInfo> DatabaseManager::listPuzzlesForRound(int roundId) const
{
    QMutexLocker locker(&m_mutex);
    QVector<PuzzleInfo> puzzles;

    QSqlQuery query(database());
    query.prepare(QStringLiteral(
        "SELECT id, round_id, sort_order, correct_answer, hint_text_key, points, template_id, "
        "puzzle_quote_slots, correct_order "
        "FROM puzzles WHERE round_id = ? ORDER BY sort_order"));
    query.addBindValue(roundId);
    if (!query.exec()) {
        emit const_cast<DatabaseManager *>(this)->databaseError(query.lastError().text());
        return puzzles;
    }

    while (query.next()) {
        PuzzleInfo info;
        info.id = query.value(0).toInt();
        info.roundId = query.value(1).toInt();
        info.sortOrder = query.value(2).toInt();
        info.correctAnswer = query.value(3).toString();
        info.hintTextKey = query.value(4).toString();
        info.points = query.value(5).toInt();
        info.templateId = query.value(6).toInt();
        info.quoteSlotsJson = query.value(7).toString();
        info.correctOrder = query.value(8).toString();
        puzzles.append(info);
    }
    return puzzles;
}

QVector<TeamInfo> DatabaseManager::listTeams() const
{
    QMutexLocker locker(&m_mutex);
    QVector<TeamInfo> teams;

    QSqlQuery query(database());
    if (!query.exec(QStringLiteral("SELECT id, team_name, score FROM current_teams ORDER BY id"))) {
        emit const_cast<DatabaseManager *>(this)->databaseError(query.lastError().text());
        return teams;
    }

    while (query.next()) {
        TeamInfo team;
        team.id = query.value(0).toInt();
        team.name = query.value(1).toString();
        team.score = query.value(2).toInt();
        teams.append(team);
    }
    return teams;
}

RoundInfo DatabaseManager::roundById(int roundId) const
{
    QMutexLocker locker(&m_mutex);
    RoundInfo info;

    QSqlQuery query(database());
    query.prepare(QStringLiteral(
        "SELECT id, title_key, rule_text_key, layout_type, timer_duration, steal_duration FROM rounds WHERE id = ?"));
    query.addBindValue(roundId);
    if (!query.exec() || !query.next()) {
        return info;
    }

    info.id = query.value(0).toInt();
    info.titleKey = query.value(1).toString();
    info.ruleTextKey = query.value(2).toString();
    info.layoutType = query.value(3).toString();
    info.timerDuration = query.value(4).toInt();
    info.stealDuration = query.value(5).toInt();
    return info;
}

PuzzleInfo DatabaseManager::puzzleById(int puzzleId) const
{
    QMutexLocker locker(&m_mutex);
    PuzzleInfo info;

    QSqlQuery query(database());
    query.prepare(QStringLiteral(
        "SELECT id, round_id, sort_order, correct_answer, hint_text_key, points, template_id, "
        "puzzle_quote_slots, correct_order FROM puzzles WHERE id = ?"));
    query.addBindValue(puzzleId);
    if (!query.exec() || !query.next()) {
        return info;
    }

    info.id = query.value(0).toInt();
    info.roundId = query.value(1).toInt();
    info.sortOrder = query.value(2).toInt();
    info.correctAnswer = query.value(3).toString();
    info.hintTextKey = query.value(4).toString();
    info.points = query.value(5).toInt();
    info.templateId = query.value(6).toInt();
    info.quoteSlotsJson = query.value(7).toString();
    info.correctOrder = query.value(8).toString();
    return info;
}

QString DatabaseManager::localizedString(const QString &key, const QString &langCode) const
{
    if (key.isEmpty()) {
        return {};
    }

    QMutexLocker locker(&m_mutex);

    QString result;
    QSqlQuery query(database());
    query.prepare(QStringLiteral(
        "SELECT translated_text FROM localization_strings WHERE string_key = ? AND lang_code = ?"));
    query.addBindValue(key);
    query.addBindValue(langCode);
    if (query.exec() && query.next()) {
        result = query.value(0).toString().trimmed();
    }

    if (!result.isEmpty()) {
        return result;
    }

    if (langCode == QStringLiteral("ru")) {
        const QString fallback = UiStringDefaults::instance().text(key);
        if (!fallback.isEmpty()) {
            return fallback;
        }
    }

    return key;
}

bool DatabaseManager::saveTeams(const QVector<TeamInfo> &teams)
{
    QMutexLocker locker(&m_mutex);
    QSqlDatabase db = database();
    if (!db.transaction()) {
        emit databaseError(db.lastError().text());
        return false;
    }

    QSqlQuery clearQuery(db);
    if (!clearQuery.exec(QStringLiteral("DELETE FROM current_teams"))) {
        emit databaseError(clearQuery.lastError().text());
        db.rollback();
        return false;
    }

    for (const TeamInfo &team : teams) {
        QSqlQuery insertQuery(db);
        insertQuery.prepare(QStringLiteral("INSERT INTO current_teams (team_name, score) VALUES (?, ?)"));
        insertQuery.addBindValue(team.name);
        insertQuery.addBindValue(team.score);
        if (!insertQuery.exec()) {
            emit databaseError(insertQuery.lastError().text());
            db.rollback();
            return false;
        }
    }

    if (!db.commit()) {
        emit databaseError(db.lastError().text());
        return false;
    }
    return true;
}

bool DatabaseManager::clearTeams()
{
    return saveTeams({});
}

GameStateSnapshot DatabaseManager::loadGameState() const
{
    QMutexLocker locker(&m_mutex);
    GameStateSnapshot state;

    QSqlQuery query(database());
    if (!query.exec(QStringLiteral(
            "SELECT current_preset_id, current_round_id, current_puzzle_id, current_turn_team_id, "
            "current_stage, current_sub_state, round_score_team_A, round_score_team_B, is_active_session "
            "FROM game_state WHERE id = 1"))
        || !query.next()) {
        return state;
    }

    state.presetId = query.value(0).toInt();
    state.roundId = query.value(1).toInt();
    state.puzzleId = query.value(2).toInt();
    state.turnTeamId = query.value(3).toInt();
    state.stage = query.value(4).toString();
    state.subState = query.value(5).toString();
    state.roundScoreTeamA = query.value(6).toInt();
    state.roundScoreTeamB = query.value(7).toInt();
    state.activeSession = query.value(8).toInt() != 0;
    return state;
}

bool DatabaseManager::saveGameState(const GameStateSnapshot &state)
{
    QMutexLocker locker(&m_mutex);

    QSqlQuery query(database());
    query.prepare(QStringLiteral(
        "UPDATE game_state SET "
        "current_preset_id = ?, current_round_id = ?, current_puzzle_id = ?, current_turn_team_id = ?, "
        "current_stage = ?, current_sub_state = ?, round_score_team_A = ?, round_score_team_B = ?, "
        "is_active_session = ? WHERE id = 1"));
    query.addBindValue(state.presetId > 0 ? state.presetId : QVariant());
    query.addBindValue(state.roundId > 0 ? state.roundId : QVariant());
    query.addBindValue(state.puzzleId > 0 ? state.puzzleId : QVariant());
    query.addBindValue(state.turnTeamId > 0 ? state.turnTeamId : QVariant());
    query.addBindValue(state.stage);
    query.addBindValue(state.subState);
    query.addBindValue(state.roundScoreTeamA);
    query.addBindValue(state.roundScoreTeamB);
    query.addBindValue(state.activeSession ? 1 : 0);
    if (!query.exec()) {
        emit databaseError(query.lastError().text());
        return false;
    }
    return true;
}

QByteArray DatabaseManager::puzzleImageData(int puzzleId, int slotIndex) const
{
    QMutexLocker locker(&m_mutex);

    QSqlQuery query(database());
    query.prepare(QStringLiteral(
        "SELECT image_data FROM puzzle_images WHERE puzzle_id = ? AND slot_index = ? LIMIT 1"));
    query.addBindValue(puzzleId);
    query.addBindValue(slotIndex);
    if (!query.exec() || !query.next()) {
        return {};
    }
    return query.value(0).toByteArray();
}

bool DatabaseManager::upsertPuzzleImage(int puzzleId, int slotIndex, const QByteArray &imageData)
{
    QMutexLocker locker(&m_mutex);

    QSqlQuery deleteQuery(database());
    deleteQuery.prepare(QStringLiteral("DELETE FROM puzzle_images WHERE puzzle_id = ? AND slot_index = ?"));
    deleteQuery.addBindValue(puzzleId);
    deleteQuery.addBindValue(slotIndex);
    if (!deleteQuery.exec()) {
        emit databaseError(deleteQuery.lastError().text());
        return false;
    }

    QSqlQuery insertQuery(database());
    insertQuery.prepare(QStringLiteral(
        "INSERT INTO puzzle_images (puzzle_id, slot_index, image_data) VALUES (?, ?, ?)"));
    insertQuery.addBindValue(puzzleId);
    insertQuery.addBindValue(slotIndex);
    insertQuery.addBindValue(imageData);
    if (!insertQuery.exec()) {
        emit databaseError(insertQuery.lastError().text());
        return false;
    }
    return true;
}

QSqlDatabase DatabaseManager::database() const
{
    return QSqlDatabase::database(m_connectionName);
}

bool DatabaseManager::execQuery(const QString &sql, const QVariantList &bindValues) const
{
    QSqlQuery query(database());
    query.prepare(sql);
    for (const QVariant &value : bindValues) {
        query.addBindValue(value);
    }
    if (!query.exec()) {
        emit const_cast<DatabaseManager *>(this)->databaseError(query.lastError().text());
        return false;
    }
    return true;
}

QSqlQuery DatabaseManager::prepareQuery(const QString &sql) const
{
    QSqlQuery query(database());
    query.prepare(sql);
    return query;
}
