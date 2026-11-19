#!/usr/bin/env python3
"""Fetch Partner Center Insights page views into Store Pulse SQLite.

Requires an authenticated Partner Center browser session cookie file at:
  %APPDATA%/StorePulse/partner_cookies.txt
Format: one Cookie header value (name=value; name2=value2).

Example (from a logged-in DevTools console on partner.microsoft.com is NOT enough —
HttpOnly cookies must be exported). Prefer running Store Pulse sync after the
desktop helper captures the session, or paste a Cookie header exported from a
browser extension.
"""

from __future__ import annotations

import json
import os
import sqlite3
import sys
import urllib.error
import urllib.parse
import urllib.request
from datetime import date, timedelta
from pathlib import Path

APP_ID_DEFAULT = "9P0HGKHQ9W19"
BASE = "https://partner.microsoft.com/storepartneranalytics/api/v1/Acquisitions/pageviews"


def cookie_path() -> Path:
    return Path(os.environ["APPDATA"]) / "StorePulse" / "partner_cookies.txt"


def db_path() -> Path:
    return Path(os.environ["APPDATA"]) / "StorePulse" / "StorePulse" / "storepulse.sqlite"


def fetch(app_id: str, start: date, end: date, cookie: str) -> list[dict]:
    qs = urllib.parse.urlencode(
        {
            "applicationId": app_id,
            "startDate": start.isoformat(),
            "endDate": end.isoformat(),
            "aggregationLevel": "day",
            "groupby": "date",
        }
    )
    req = urllib.request.Request(
        f"{BASE}?{qs}",
        headers={
            "Accept": "application/json",
            "Cookie": cookie,
            "User-Agent": "StorePulse/1.0",
        },
        method="GET",
    )
    with urllib.request.urlopen(req, timeout=120) as resp:
        payload = json.loads(resp.read().decode("utf-8"))
    return payload.get("Value") or []


def upsert(db: Path, store_id: str, rows: list[dict]) -> tuple[int, int]:
    con = sqlite3.connect(str(db))
    con.execute(
        """
CREATE TABLE IF NOT EXISTS insights_pageviews (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    store_id TEXT NOT NULL,
    date_utc TEXT NOT NULL,
    pageviews_count INTEGER NOT NULL DEFAULT 0,
    ingested_at TEXT NOT NULL DEFAULT (strftime('%Y-%m-%dT%H:%M:%fZ','now')),
    UNIQUE (store_id, date_utc)
)
"""
    )
    total = 0
    for row in rows:
        d = str(row.get("date", ""))[:10]
        n = int(row.get("pageviewsCount") or 0)
        if not d:
            continue
        con.execute(
            "INSERT OR REPLACE INTO insights_pageviews(store_id,date_utc,pageviews_count) VALUES (?,?,?)",
            (store_id, d, n),
        )
        total += n
    con.commit()
    return len(rows), total


def main() -> int:
    cookie_file = cookie_path()
    if not cookie_file.exists():
        print(f"Missing cookie file: {cookie_file}", file=sys.stderr)
        return 2
    cookie = cookie_file.read_text(encoding="utf-8").strip()
    if not cookie:
        print("Cookie file is empty", file=sys.stderr)
        return 2

    app_id = sys.argv[1] if len(sys.argv) > 1 else APP_ID_DEFAULT
    end = date.today()
    start = end - timedelta(days=90)
    try:
        rows = fetch(app_id, start, end, cookie)
    except urllib.error.HTTPError as e:
        print(f"HTTP {e.code}: {e.read()[:300]!r}", file=sys.stderr)
        return 1
    except Exception as e:
        print(f"Fetch failed: {e}", file=sys.stderr)
        return 1

    count, total = upsert(db_path(), app_id, rows)
    print(f"OK rows={count} pageviews={total} app={app_id}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
