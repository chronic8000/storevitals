#!/usr/bin/env python3
"""Phase 1 API probe + light SQLite checks for AQEMU (9P0HGKHQ9W19).

Uses Entra client-credentials from StorePulse.ini + WinCred secret.
Does NOT replace Full sync in the app — validates Partner Center returns
failuredetails / stack / reviews hardware / channel referrers, and that
migration 007 tables exist.
"""

from __future__ import annotations

import configparser
import ctypes
import json
import os
import sqlite3
import sys
import time
import urllib.error
import urllib.parse
import urllib.request
from ctypes import wintypes
from datetime import date, timedelta
from pathlib import Path

APP_ID = "9P0HGKHQ9W19"
ANALYTICS = "https://manage.devcenter.microsoft.com/v1.0/my/analytics"
TOKEN_URL_TMPL = "https://login.microsoftonline.com/{tenant}/oauth2/v2.0/token"
SCOPE = "https://manage.devcenter.microsoft.com/.default"


class CREDENTIAL(ctypes.Structure):
    _fields_ = [
        ("Flags", wintypes.DWORD),
        ("Type", wintypes.DWORD),
        ("TargetName", wintypes.LPWSTR),
        ("Comment", wintypes.LPWSTR),
        ("LastWritten", wintypes.FILETIME),
        ("CredentialBlobSize", wintypes.DWORD),
        ("CredentialBlob", ctypes.POINTER(ctypes.c_char)),
        ("Persist", wintypes.DWORD),
        ("AttributeCount", wintypes.DWORD),
        ("Attributes", ctypes.c_void_p),
        ("TargetAlias", wintypes.LPWSTR),
        ("UserName", wintypes.LPWSTR),
    ]


def read_secret(target: str = "StorePulse/EntraClientSecret") -> str:
    adv = ctypes.windll.advapi32
    cred = ctypes.POINTER(CREDENTIAL)()
    if not adv.CredReadW(target, 1, 0, ctypes.byref(cred)):
        raise RuntimeError("WinCred miss for " + target)
    try:
        blob = ctypes.string_at(cred.contents.CredentialBlob, cred.contents.CredentialBlobSize)
        return blob.decode("utf-16-le").rstrip("\x00")
    finally:
        adv.CredFree(cred)


def load_cfg() -> tuple[str, str]:
    ini = Path(os.environ["APPDATA"]) / "StorePulse" / "StorePulse.ini"
    cp = configparser.ConfigParser()
    text = ini.read_text(encoding="utf-8-sig")
    cp.read_string(text)
    return cp.get("auth", "tenantId"), cp.get("auth", "clientId")


def db_path() -> Path:
    return Path(os.environ["APPDATA"]) / "StorePulse" / "StorePulse" / "storepulse.sqlite"


def token(tenant: str, client_id: str, secret: str) -> str:
    body = urllib.parse.urlencode(
        {
            "client_id": client_id,
            "client_secret": secret,
            "scope": SCOPE,
            "grant_type": "client_credentials",
        }
    ).encode()
    req = urllib.request.Request(
        TOKEN_URL_TMPL.format(tenant=tenant),
        data=body,
        headers={"Content-Type": "application/x-www-form-urlencoded"},
        method="POST",
    )
    with urllib.request.urlopen(req, timeout=60) as resp:
        return json.loads(resp.read().decode())["access_token"]


def get_json(tok: str, path: str, params: dict) -> dict:
    qs = urllib.parse.urlencode({k: v for k, v in params.items() if v is not None and v != ""})
    url = f"{ANALYTICS}/{path}?{qs}"
    req = urllib.request.Request(
        url,
        headers={"Authorization": f"Bearer {tok}", "Accept": "application/json"},
        method="GET",
    )
    try:
        with urllib.request.urlopen(req, timeout=120) as resp:
            return json.loads(resp.read().decode("utf-8"))
    except urllib.error.HTTPError as e:
        body = e.read().decode("utf-8", errors="replace")[:400]
        raise RuntimeError(f"HTTP {e.code} {path}: {body}") from e


def values(payload: dict) -> list:
    return payload.get("Value") or payload.get("value") or []


def main() -> int:
    tenant, client_id = load_cfg()
    secret = read_secret()
    tok = token(tenant, client_id, secret)
    end = date.today()
    start = end - timedelta(days=29)
    report: dict = {"app": APP_ID, "window": f"{start}..{end}"}

    hits = get_json(
        tok,
        "failurehits",
        {
            "applicationId": APP_ID,
            "startDate": start.isoformat(),
            "endDate": end.isoformat(),
            "groupby": "failureName,failureHash,osArchitecture",
            "orderby": "date",
            "top": 100,
        },
    )
    hit_rows = values(hits)
    hashes = []
    for r in hit_rows:
        h = r.get("failureHash") or r.get("FailureHash")
        if h and h not in hashes:
            hashes.append(h)
    report["failurehits"] = len(hit_rows)
    report["unique_hashes"] = len(hashes)

    detail_sample = None
    detail_count = 0
    for h in hashes[:5]:
        time.sleep(4.2)
        det = get_json(
            tok,
            "failuredetails",
            {
                "applicationId": APP_ID,
                "failureHash": h,
                "startDate": start.isoformat(),
                "endDate": end.isoformat(),
                "top": 50,
            },
        )
        rows = values(det)
        detail_count += len(rows)
        if rows and detail_sample is None:
            detail_sample = rows[0]
    report["failuredetails_rows"] = detail_count
    if detail_sample:
        report["detail_keys"] = sorted(detail_sample.keys())
        report["sample_device"] = detail_sample.get("deviceModel") or detail_sample.get("DeviceModel")
        report["sample_cpu"] = detail_sample.get("cpuName") or detail_sample.get("CpuName")
        report["sample_arch"] = detail_sample.get("osArchitecture") or detail_sample.get("OsArchitecture")
        report["sample_flight"] = detail_sample.get("flightRing") or detail_sample.get("FlightRing")
        report["sample_osRelease"] = detail_sample.get("osRelease") or detail_sample.get("OsRelease")
        report["sample_cabId"] = detail_sample.get("cabId") or detail_sample.get("CabId")
        report["sample_cabDownloadable"] = detail_sample.get("cabDownloadable") or detail_sample.get(
            "CabDownloadable"
        )

    cab = report.get("sample_cabId")
    if cab:
        time.sleep(4.2)
        try:
            stack = get_json(
                tok,
                "stacktrace",
                {"applicationId": APP_ID, "cabId": cab},
            )
            frames = values(stack)
            report["stack_frames"] = len(frames)
            if frames:
                report["stack_sample"] = frames[0]
        except Exception as ex:  # noqa: BLE001
            report["stack_error"] = str(ex)

    time.sleep(4.2)
    reviews = get_json(
        tok,
        "reviews",
        {"applicationId": APP_ID, "startDate": (end - timedelta(days=365 * 3)).isoformat(), "endDate": end.isoformat(), "top": 20},
    )
    rev_rows = values(reviews)
    report["reviews"] = len(rev_rows)
    if rev_rows:
        report["review_keys"] = sorted(rev_rows[0].keys())
        r0 = rev_rows[0]
        report["review_device"] = r0.get("deviceModel") or r0.get("DeviceModel")
        report["review_ram"] = r0.get("deviceRAM") or r0.get("DeviceRAM")
        report["review_topics"] = r0.get("topicTags") or r0.get("topics")

    time.sleep(4.2)
    ch = get_json(
        tok,
        "appchannelconversions",
        {
            "applicationId": APP_ID,
            "startDate": (end - timedelta(days=89)).isoformat(),
            "endDate": end.isoformat(),
            "groupby": "date,channelType,referrerUriDomain,customCampaignId,deviceType,market",
            "top": 200,
        },
    )
    ch_rows = values(ch)
    report["channel_rows"] = len(ch_rows)
    domains = {
        (r.get("referrerUriDomain") or r.get("ReferrerUriDomain") or "").strip()
        for r in ch_rows
    }
    domains.discard("")
    camps = {
        (r.get("customCampaignId") or r.get("CustomCampaignId") or r.get("campaignId") or "").strip()
        for r in ch_rows
    }
    camps.discard("")
    report["referrer_domains"] = sorted(domains)[:20]
    report["campaigns"] = sorted(camps)[:20]

    con = sqlite3.connect(str(db_path()))
    ver = con.execute("SELECT MAX(version) FROM schema_migrations").fetchone()[0]
    report["schema_version"] = ver
    cols = [r[1] for r in con.execute("PRAGMA table_info(failure_details)").fetchall()]
    report["failure_details_cols"] = cols
    report["failure_details_count"] = con.execute(
        "SELECT COUNT(*) FROM failure_details WHERE store_id=?", (APP_ID,)
    ).fetchone()[0]
    report["reviews_with_device"] = con.execute(
        "SELECT COUNT(*) FROM reviews WHERE store_id=? AND device_model!=''", (APP_ID,)
    ).fetchone()[0]
    report["channel_referrers"] = con.execute(
        "SELECT COUNT(*) FROM channel_conversions WHERE store_id=? AND referrer_uri_domain!=''",
        (APP_ID,),
    ).fetchone()[0]
    report["stacks_in_db"] = con.execute(
        "SELECT COUNT(*) FROM failure_details WHERE store_id=? AND length(stack_trace)>0",
        (APP_ID,),
    ).fetchone()[0]
    con.close()

    out = Path(__file__).with_name("phase1_verify_report.json")
    out.write_text(json.dumps(report, indent=2), encoding="utf-8")
    print(json.dumps(report, indent=2))
    print(f"\nWrote {out}")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as e:  # noqa: BLE001
        print("ERROR:", e, file=sys.stderr)
        raise SystemExit(1)
