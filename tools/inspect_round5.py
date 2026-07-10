import os
import sqlite3
import sys

sys.stdout.reconfigure(encoding="utf-8")

db = os.path.join(os.environ["APPDATA"], "WhereLogic", "WhereLogicGame", "wherelogic.db")
c = sqlite3.connect(db)

ids = [2, 307, 304, 301, 298]
for pid in ids:
    row = c.execute(
        "SELECT id, round_id, sort_order, correct_answer, hint_text_key FROM puzzles WHERE id=?",
        (pid,),
    ).fetchone()
    hint = c.execute(
        "SELECT translated_text FROM localization_strings WHERE string_key=? AND lang_code='ru'",
        (f"puzzle.hint.p{pid}",),
    ).fetchone()
    masks = c.execute(
        "SELECT COUNT(*) FROM puzzle_masks pm JOIN puzzles p ON p.id=pm.puzzle_id WHERE p.id=?",
        (pid,),
    ).fetchone()
    print("puzzle", pid, "meta", row)
    print("  hint ru:", hint)
    print("  masks:", masks)

c.close()
