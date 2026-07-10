import os
import sqlite3
import sys

sys.stdout.reconfigure(encoding="utf-8")

db = os.path.join(os.environ["APPDATA"], "WhereLogic", "WhereLogicGame", "wherelogic.db")
c = sqlite3.connect(db)

print("First 10 puzzles in round 5 (as editor shows):")
for i, row in enumerate(
    c.execute(
        "SELECT id, sort_order, correct_answer FROM puzzles WHERE round_id=5 ORDER BY sort_order, id LIMIT 10"
    ),
    1,
):
    pid = row[0]
    hint = c.execute(
        "SELECT translated_text FROM localization_strings WHERE string_key=? AND lang_code='ru'",
        (f"puzzle.hint.p{pid}",),
    ).fetchone()
    print(f"  UI #{i}: id={pid} sort={row[1]} answer={row[2]!r} hint={hint[0] if hint else ''!r}")

c.close()
