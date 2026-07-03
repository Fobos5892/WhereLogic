#pragma once

#include <QString>

namespace RestApi {

constexpr int API_VERSION = 1;
constexpr int DEFAULT_SERVER_PORT = 8765;
constexpr int HEARTBEAT_INTERVAL_MS = 20000;
constexpr int REMOTE_IDLE_TIMEOUT_MS = 40000;
constexpr int REMOTE_IDLE_CHECK_MS = 5000;
constexpr int PUZZLE_POLL_INTERVAL_MS = 2000;

inline const QString AUTH_HEADER = QStringLiteral("X-Auth-Token");

namespace Paths {
inline const QString Authenticate = QStringLiteral("/api/auth");
inline const QString Heartbeat = QStringLiteral("/api/heartbeat");
inline const QString CurrentPuzzle = QStringLiteral("/api/current_puzzle");
inline const QString Action = QStringLiteral("/api/action");
inline const QString SubmitText = QStringLiteral("/api/submit_text");
} // namespace Paths

namespace Actions {
inline const QString Ready = QStringLiteral("READY");
inline const QString TransferTurn = QStringLiteral("TRANSFER_TURN");
inline const QString ResolveA = QStringLiteral("RESOLVE_A");
inline const QString ResolveB = QStringLiteral("RESOLVE_B");
inline const QString RejectAll = QStringLiteral("REJECT_ALL");
} // namespace Actions

} // namespace RestApi
