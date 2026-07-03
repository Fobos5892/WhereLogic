#!/usr/bin/env python3
"""
WhereLogic — загрузка AI-зависимостей (whisper.cpp, модели, OpenCV).

Читает config/deps_manifest.json. Запуск из корня репозитория:

  python scripts/fetch_ai_deps.py --list
  python scripts/fetch_ai_deps.py --all
  python scripts/fetch_ai_deps.py --group whisper
  python scripts/fetch_ai_deps.py --id whisper_model_tiny
"""

from __future__ import annotations

import argparse
import json
import os
import shutil
import subprocess
import sys
import tempfile
import urllib.error
import urllib.request
from pathlib import Path
from typing import Callable, Iterable

ROOT = Path(__file__).resolve().parent.parent
MANIFEST_PATH = ROOT / "config" / "deps_manifest.json"
CHUNK = 256 * 1024
USER_AGENT = "WhereLogic-Setup/1.0"


def load_manifest() -> dict:
    with MANIFEST_PATH.open(encoding="utf-8") as f:
        return json.load(f)


def repo_path(rel: str) -> Path:
    return ROOT / rel.replace("/", os.sep)


def is_installed(item: dict) -> bool:
    if item.get("kind") == "manual":
        return False
    verify = item.get("verify_path", "")
    return bool(verify) and repo_path(verify).exists()


def platform_ok(item: dict) -> bool:
    platforms = item.get("platforms")
    if not platforms:
        return True
    if sys.platform == "win32":
        return "win32" in platforms
    if sys.platform == "darwin":
        return "darwin" in platforms
    return "linux" in platforms


def print_progress(done: int, total: int, prefix: str) -> None:
    if total > 0:
        pct = min(100, done * 100 // total)
        bar = "=" * (pct // 2) + "-" * (50 - pct // 2)
        sys.stdout.write(f"\r{prefix} [{bar}] {pct:3d}% ({_fmt_bytes(done)}/{_fmt_bytes(total)})")
    else:
        sys.stdout.write(f"\r{prefix} {_fmt_bytes(done)}")
    sys.stdout.flush()


def _fmt_bytes(n: int) -> str:
    if n < 1024:
        return f"{n} B"
    if n < 1024 * 1024:
        return f"{n / 1024:.1f} KB"
    return f"{n / 1024 / 1024:.1f} MB"


def download_http(
    url: str,
    dest: Path,
    expected_size: int = 0,
    on_progress: Callable[[int, int], None] | None = None,
) -> None:
    dest.parent.mkdir(parents=True, exist_ok=True)
    tmp = dest.with_suffix(dest.suffix + ".part")
    req = urllib.request.Request(url, headers={"User-Agent": USER_AGENT})
    try:
        with urllib.request.urlopen(req, timeout=60) as resp:
            total = int(resp.headers.get("Content-Length", 0)) or expected_size
            done = 0
            with tmp.open("wb") as out:
                while True:
                    chunk = resp.read(CHUNK)
                    if not chunk:
                        break
                    out.write(chunk)
                    done += len(chunk)
                    if on_progress:
                        on_progress(done, total)
    except urllib.error.URLError as e:
        if tmp.exists():
            tmp.unlink()
        raise RuntimeError(f"HTTP download failed: {e}") from e

    if on_progress:
        sys.stdout.write("\n")
    tmp.replace(dest)


def git_clone(url: str, dest: Path) -> None:
    if dest.exists():
        shutil.rmtree(dest)
    dest.parent.mkdir(parents=True, exist_ok=True)
    subprocess.run(
        ["git", "clone", "--depth", "1", url, str(dest)],
        check=True,
        cwd=ROOT,
    )


def install_opencv_windows(item: dict, on_progress: Callable[[int, int], None] | None) -> None:
    version = item.get("version", "4.10.0")
    url = item["url"]
    prebuilt = repo_path(item["destination"])

    with tempfile.TemporaryDirectory(prefix="opencv-fetch-") as tmpdir:
        tmp = Path(tmpdir)
        installer = tmp / f"opencv-{version}-windows.exe"
        extract_root = tmp / "extract"

        print(f"  Downloading OpenCV {version} installer...")
        download_http(
            url,
            installer,
            item.get("size_bytes", 0),
            on_progress,
        )

        print("  Extracting (self-extracting 7z)...")
        extract_root.mkdir()
        subprocess.run(
            [str(installer), f"-o{extract_root}", "-y"],
            check=True,
        )

        src_include = extract_root / "opencv" / "build" / "include"
        src_lib = extract_root / "opencv" / "build" / "x64" / "vc16" / "lib"
        src_bin = extract_root / "opencv" / "build" / "x64" / "vc16" / "bin"

        if not src_include.exists():
            raise RuntimeError("OpenCV extract layout unexpected — check installer version")

        msvc_lib = prebuilt / "x64" / "msvc" / "lib"
        msvc_bin = prebuilt / "x64" / "msvc" / "bin"
        msvc_lib.mkdir(parents=True, exist_ok=True)
        msvc_bin.mkdir(parents=True, exist_ok=True)

        if (prebuilt / "include").exists():
            shutil.rmtree(prebuilt / "include")
        shutil.copytree(src_include, prebuilt / "include", dirs_exist_ok=True)
        for lib in src_lib.glob("*.lib"):
            shutil.copy2(lib, msvc_lib / lib.name)
        for dll in src_bin.glob("opencv_world*.dll"):
            shutil.copy2(dll, msvc_bin / dll.name)

        # Remove legacy flat layout if present
        for legacy in ("lib", "bin"):
            leg = prebuilt / legacy
            if leg.exists():
                shutil.rmtree(leg)

    print(f"  OpenCV MSVC prebuilt -> {prebuilt / 'x64' / 'msvc'}")
    print("  Use Qt MSVC 64-bit kit. MinGW needs separate build in x64/mingw/.")


def install_item(item: dict, force: bool = False) -> bool:
    kind = item.get("kind", "http")
    label = item.get("label", item.get("id", "?"))

    if kind == "manual":
        hint = item.get("manual_hint", "Install manually.")
        print(f"[manual] {label}\n  {hint}")
        return False

    if not platform_ok(item):
        print(f"[skip] {label} — not for this platform")
        return False

    if is_installed(item) and not force:
        print(f"[ok] {label} — already present")
        return True

    print(f"[fetch] {label}")

    if kind == "git":
        git_clone(item["url"], repo_path(item["destination"]))
    elif kind == "http":
        dest = repo_path(item["destination"])

        def prog(done: int, total: int) -> None:
            print_progress(done, total, "  ")

        download_http(
            item["url"],
            dest,
            item.get("size_bytes", 0),
            prog,
        )
        print(f"  Saved: {dest}")
    elif kind == "opencv_windows":
        def prog(done: int, total: int) -> None:
            print_progress(done, total, "  ")

        install_opencv_windows(item, prog)
    else:
        raise RuntimeError(f"Unknown kind: {kind}")

    if not is_installed(item):
        raise RuntimeError(f"Verify failed after install: {item.get('verify_path')}")

    print(f"[done] {label}")
    return True


def filter_items(
    components: Iterable[dict],
    *,
    group: str | None,
    ids: list[str] | None,
    all_flag: bool,
) -> list[dict]:
    items = list(components)
    if ids:
        id_set = set(ids)
        return [c for c in items if c.get("id") in id_set]
    if group:
        return [c for c in items if group in c.get("groups", [])]
    if all_flag:
        return [c for c in items if c.get("kind") != "manual"]
    return []


def cmd_list(components: list[dict]) -> None:
    print(f"Manifest: {MANIFEST_PATH}\n")
    for item in components:
        status = "installed" if is_installed(item) else item.get("kind", "?")
        plat = ""
        if item.get("platforms"):
            plat = f" [{','.join(item['platforms'])}]"
        print(f"  {item.get('id')}: {item.get('label')}{plat} — {status}")
        if item.get("kind") == "manual" and item.get("manual_hint"):
            print(f"    -> {item['manual_hint']}")


def main() -> int:
    parser = argparse.ArgumentParser(description="Fetch WhereLogic AI dependencies")
    parser.add_argument("--list", action="store_true", help="Show components and status")
    parser.add_argument("--all", action="store_true", help="Download all auto components")
    parser.add_argument("--group", choices=["whisper", "opencv", "ai", "network"])
    parser.add_argument("--id", action="append", dest="ids", metavar="ID")
    parser.add_argument("--force", action="store_true", help="Re-download even if present")
    args = parser.parse_args()

    if not MANIFEST_PATH.exists():
        print(f"Missing {MANIFEST_PATH}", file=sys.stderr)
        return 1

    manifest = load_manifest()
    components = manifest.get("components", [])

    if args.list or not (args.all or args.group or args.ids):
        cmd_list(components)
        if not (args.all or args.group or args.ids):
            print("\nUse --all, --group whisper|opencv|ai, or --id <id>")
        if args.list and not (args.all or args.group or args.ids):
            return 0

    selected = filter_items(
        components,
        group=args.group,
        ids=args.ids,
        all_flag=args.all,
    )
    if not selected:
        print("Nothing selected.", file=sys.stderr)
        return 1

    errors = 0
    for item in selected:
        try:
            install_item(item, force=args.force)
        except (RuntimeError, subprocess.CalledProcessError, OSError) as e:
            print(f"[error] {item.get('label')}: {e}", file=sys.stderr)
            errors += 1

    if errors:
        return 1
    print("\nAll selected components ready.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
