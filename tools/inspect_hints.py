import os
import sqlite3

db = os.path.join(os.environ["APPDATA"], "WhereLogic", "WhereLogicGame", "wherelogic.db")
print("DB:", db, "exists:", os.path.exists(db))
if not os.path.exists(db):
    raise SystemExit(1)

c = sqlite3.connect(db)
print("\n=== puzzles ===")
for row in c.execute(
    "SELECT id, round_id, sort_order, correct_answer, hint_text_key FROM puzzles ORDER BY round_id, sort_order"
):
    print(row)

print("\n=== puzzle hints (localization) ===")
for row in c.execute(
    "SELECT string_key, lang_code, translated_text FROM localization_strings "
    "WHERE string_key LIKE 'puzzle.hint%' ORDER BY string_key, lang_code"
):
    print(row)

print("\n=== FULL_MASK round ===")
for row in c.execute("SELECT id, layout_type FROM rounds WHERE layout_type = 'FULL_MASK'"):
    print("round", row)

c.close()
