#pragma once

#include <QString>

namespace GameConstants {

// Network timing (aligned with shared/api/RestApiConstants.h)
constexpr int PIN_ROTATION_INTERVAL_MS = 30000;
constexpr int HEARTBEAT_INTERVAL_MS = 20000;
constexpr int REMOTE_IDLE_TIMEOUT_MS = 40000;
constexpr int PUZZLE_POLL_INTERVAL_MS = 2000;

constexpr int DEFAULT_SERVER_PORT = 8765;
constexpr int PIN_LENGTH = 5;

// Timer policy
constexpr int STEAL_MIN_SECONDS = 15;
constexpr double CRITICAL_TIMER_THRESHOLD_SECONDS = 10.0;
constexpr int MAX_ROUND_STARS = 5;
constexpr int MAX_PUZZLES_PER_ROUND = 5;

// Teams
inline const QString TEAM_A = QStringLiteral("Team_A");
inline const QString TEAM_B = QStringLiteral("Team_B");

// Game stages (persisted in game_state.current_stage)
namespace Stage {
inline const QString Welcome = QStringLiteral("STAGE_WELCOME");
inline const QString TeamSetup = QStringLiteral("STAGE_TEAM_SETUP");
inline const QString ClosedCards = QStringLiteral("STAGE_CLOSED_CARDS");
inline const QString MainTurn = QStringLiteral("STAGE_MAIN_TURN");
inline const QString StealTurn = QStringLiteral("STAGE_STEAL_TURN");
inline const QString Evaluating = QStringLiteral("STAGE_EVALUATING");
inline const QString MissingReveal = QStringLiteral("STAGE_MISSING_REVEAL");
inline const QString Resolution = QStringLiteral("STAGE_RESOLUTION");
inline const QString InterRound = QStringLiteral("STAGE_INTER_ROUND");
inline const QString FinalVictory = QStringLiteral("STAGE_FINAL_VICTORY");
} // namespace Stage

// Sub-states (persisted in game_state.current_sub_state)
namespace SubState {
inline const QString MainTurn = QStringLiteral("MAIN_TURN");
inline const QString StealTurn = QStringLiteral("STEAL_TURN");
inline const QString AwaitingReady = QStringLiteral("AWAITING_READY");
inline const QString AwaitingResolve = QStringLiteral("AWAITING_RESOLVE");
} // namespace SubState

// Layout types (rounds.layout_type)
namespace LayoutType {
inline const QString Standard = QStringLiteral("STANDARD");
inline const QString Equation = QStringLiteral("EQUATION");
inline const QString FullMask = QStringLiteral("FULL_MASK");
inline const QString SingleHybrid = QStringLiteral("SINGLE_HYBRID");
inline const QString Chronology = QStringLiteral("CHRONOLOGY");
inline const QString TextOnly = QStringLiteral("TEXT_ONLY");
inline const QString BlitzStandard = QStringLiteral("BLITZ_STANDARD");
} // namespace LayoutType

inline int stealDurationSeconds(int baseTimerSeconds)
{
    const int computed = baseTimerSeconds / 4;
    return computed < STEAL_MIN_SECONDS ? STEAL_MIN_SECONDS : computed;
}

} // namespace GameConstants
