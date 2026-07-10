import os
import sqlite3
import sys

sys.stdout.reconfigure(encoding="utf-8")

db = os.path.join(os.environ["APPDATA"], "WhereLogic", "WhereLogicGame", "wherelogic.db")
print("DB:", db)

c = sqlite3.connect(db)
c.row_factory = sqlite3.Row

DEMO_ANSWERS = {
    "",
    "Ответ",
    "Париж",
    "Фен",
    "Свадебный букет",
}
DEMO_HINTS = {
    "",
    "Подсказка",
    "Семейный праздник",
    "Любимая страна мамы",
    "Молодёжный сленг",
    "Фирменная выпечка",
    "Любимый сериал",
    "Бытовой прибор",
    "Быстрая связь",
}

MAX_PER_ROUND = 10


def puzzle_stats(pid):
    row = c.execute(
        """
        SELECT p.id, p.round_id, p.sort_order, p.correct_answer,
               ls.translated_text AS hint_ru,
               (SELECT COUNT(*) FROM puzzle_images pi
                WHERE pi.puzzle_id = p.id AND length(pi.image_data) > 0) AS image_count,
               (SELECT COUNT(*) FROM puzzle_masks pm WHERE pm.puzzle_id = p.id) AS mask_count
        FROM puzzles p
        LEFT JOIN localization_strings ls
          ON ls.string_key = ('puzzle.hint.p' || p.id) AND ls.lang_code = 'ru'
        WHERE p.id = ?
        """,
        (pid,),
    ).fetchone()
    return row


def is_meaningful(row):
    answer = (row["correct_answer"] or "").strip()
    hint = (row["hint_ru"] or "").strip()
    if row["mask_count"] > 0:
        return True
    if answer not in DEMO_ANSWERS and hint not in DEMO_HINTS:
        return True
    return False


rows = [puzzle_stats(row["id"]) for row in c.execute("SELECT id FROM puzzles ORDER BY id")]
meaningful = [r for r in rows if is_meaningful(r)]
garbage = [r for r in rows if not is_meaningful(r)]

print(f"Before: {len(rows)} puzzles, meaningful={len(meaningful)}, garbage={len(garbage)}")

# Keep one canonical puzzle per round/sort_order among meaningful rows.
best_by_slot = {}
for row in meaningful:
    key = (row["round_id"], row["sort_order"])
    score = (row["mask_count"], row["image_count"], -row["id"])
    prev = best_by_slot.get(key)
    if prev is None or score > prev[0]:
        best_by_slot[key] = (score, row["id"])

keep_ids = set()
for round_id in sorted({row["round_id"] for row in meaningful}):
    slots = sorted(
        [slot for slot, (_, pid) in best_by_slot.items() if slot[0] == round_id],
        key=lambda s: s[1],
    )
    for _, sort_order in slots[:MAX_PER_ROUND]:
        keep_ids.add(best_by_slot[(round_id, sort_order)][1])

delete_ids = sorted({row["id"] for row in rows} - keep_ids)
print(f"Keeping: {len(keep_ids)}")
print(f"Deleting: {len(delete_ids)}")

print("\nAll kept puzzles:")
for row in sorted(
    (puzzle_stats(pid) for pid in keep_ids),
    key=lambda r: (r["round_id"], r["sort_order"], r["id"]),
):
    print(
        f"  round={row['round_id']} #{row['sort_order']} id={row['id']} "
        f"answer={row['correct_answer']!r} hint={(row['hint_ru'] or '')!r}"
    )

if "--apply" not in sys.argv:
    print("\nDry run only. Re-run with --apply to delete.")
    c.close()
    raise SystemExit(0)

c.execute("PRAGMA foreign_keys = OFF")
c.execute("BEGIN")

for pid in delete_ids:
    hint_key = f"puzzle.hint.p{pid}"
    c.execute("DELETE FROM puzzle_masks WHERE puzzle_id = ?", (pid,))
    c.execute("DELETE FROM puzzle_images WHERE puzzle_id = ?", (pid,))
    c.execute("DELETE FROM localization_strings WHERE string_key = ?", (hint_key,))
    c.execute("DELETE FROM puzzles WHERE id = ?", (pid,))

c.execute(
    """
    DELETE FROM localization_strings
    WHERE string_key LIKE 'puzzle.hint.p%'
      AND string_key NOT IN (SELECT 'puzzle.hint.p' || id FROM puzzles)
    """
)

# Fix game_state if it pointed at a deleted puzzle in the same round/sort slot.
state = c.execute(
    "SELECT current_puzzle_id, current_round_id FROM game_state WHERE id = 1"
).fetchone()
if state and state["current_puzzle_id"] in delete_ids:
    replacement = c.execute(
        """
        SELECT id FROM puzzles
        WHERE round_id = ?
        ORDER BY sort_order, id
        LIMIT 1
        """,
        (state["current_round_id"],),
    ).fetchone()
    new_pid = replacement["id"] if replacement else 0
    c.execute(
        "UPDATE game_state SET current_puzzle_id = ? WHERE id = 1",
        (new_pid,),
    )
    print(f"Updated game_state puzzle id -> {new_pid}")

c.execute("COMMIT")
c.execute("PRAGMA foreign_keys = ON")

print("\nAfter:")
print(" puzzles:", c.execute("SELECT COUNT(*) FROM puzzles").fetchone()[0])
print(" puzzle_images:", c.execute("SELECT COUNT(*) FROM puzzle_images").fetchone()[0])
print(" puzzle_masks:", c.execute("SELECT COUNT(*) FROM puzzle_masks").fetchone()[0])
print(" hint strings:", c.execute("SELECT COUNT(*) FROM localization_strings WHERE string_key LIKE 'puzzle.hint.p%'").fetchone()[0])

c.close()
print("Done.")
