-- Store Pulse schema v1
-- All UI reads from these tables; network writes only via repositories.

PRAGMA foreign_keys = ON;

CREATE TABLE IF NOT EXISTS schema_migrations (
    version     INTEGER PRIMARY KEY,
    applied_at  TEXT NOT NULL DEFAULT (strftime('%Y-%m-%dT%H:%M:%fZ', 'now')),
    name        TEXT NOT NULL
);

CREATE TABLE IF NOT EXISTS apps (
    store_id        TEXT PRIMARY KEY,          -- e.g. 9P0HGKHQ9W19
    display_name    TEXT NOT NULL,
    package_family  TEXT,
    is_active       INTEGER NOT NULL DEFAULT 1,
    first_seen_at   TEXT,
    updated_at      TEXT NOT NULL DEFAULT (strftime('%Y-%m-%dT%H:%M:%fZ', 'now'))
);

-- Daily (or hourly via scheduled reports) acquisition facts.
-- Natural key supports 10-day restatement upserts.
CREATE TABLE IF NOT EXISTS acquisitions (
    id                  INTEGER PRIMARY KEY AUTOINCREMENT,
    store_id            TEXT NOT NULL REFERENCES apps(store_id) ON DELETE CASCADE,
    date_utc            TEXT NOT NULL,           -- YYYY-MM-DD or YYYY-MM-DDTHH:00:00Z
    granularity         TEXT NOT NULL DEFAULT 'daily'
                            CHECK (granularity IN ('hourly', 'daily')),
    acquisition_type    TEXT NOT NULL DEFAULT '', -- e.g. Free, Trial, Paid, IAP
    market              TEXT NOT NULL DEFAULT '',
    device_type         TEXT NOT NULL DEFAULT '',
    age_group           TEXT NOT NULL DEFAULT '',
    gender              TEXT NOT NULL DEFAULT '',
    store_client        TEXT NOT NULL DEFAULT '',
    channel             TEXT NOT NULL DEFAULT '',
    campaign_id         TEXT NOT NULL DEFAULT '',
    quantity            INTEGER NOT NULL DEFAULT 0,
    source              TEXT NOT NULL DEFAULT 'analytics'
                            CHECK (source IN ('analytics', 'scheduled_report')),
    ingested_at         TEXT NOT NULL DEFAULT (strftime('%Y-%m-%dT%H:%M:%fZ', 'now')),
    restated_at         TEXT,
    UNIQUE (
        store_id, date_utc, granularity, acquisition_type, market,
        device_type, age_group, gender, store_client, channel, campaign_id, source
    )
);

CREATE INDEX IF NOT EXISTS idx_acquisitions_store_date
    ON acquisitions(store_id, date_utc);

CREATE INDEX IF NOT EXISTS idx_acquisitions_restatement
    ON acquisitions(store_id, date_utc, granularity);

-- Crash / failure hits from analytics/failurehits.
CREATE TABLE IF NOT EXISTS failure_hits (
    id                  INTEGER PRIMARY KEY AUTOINCREMENT,
    store_id            TEXT NOT NULL REFERENCES apps(store_id) ON DELETE CASCADE,
    date_utc            TEXT NOT NULL,           -- event day (UTC)
    failure_name        TEXT NOT NULL DEFAULT '',
    failure_hash        TEXT NOT NULL DEFAULT '',
    failure_stack_hash  TEXT NOT NULL DEFAULT '',
    os_version          TEXT NOT NULL DEFAULT '',
    os_architecture     TEXT NOT NULL DEFAULT '', -- x64 / arm64 / x86 / ...
    package_version     TEXT NOT NULL DEFAULT '',
    event_type          TEXT NOT NULL DEFAULT '', -- crash, hang, etc.
    event_count         INTEGER NOT NULL DEFAULT 0,
    cab_id              TEXT NOT NULL DEFAULT '',
    stack_trace         TEXT,                    -- may be large; not PII per Partner Center payload
    first_seen_at       TEXT,
    last_seen_at        TEXT,
    ingested_at         TEXT NOT NULL DEFAULT (strftime('%Y-%m-%dT%H:%M:%fZ', 'now')),
    restated_at         TEXT,
    UNIQUE (
        store_id, date_utc, failure_hash, failure_stack_hash,
        os_version, os_architecture, package_version, event_type, cab_id
    )
);

CREATE INDEX IF NOT EXISTS idx_failure_hits_store_date
    ON failure_hits(store_id, date_utc);

CREATE INDEX IF NOT EXISTS idx_failure_hits_spike
    ON failure_hits(store_id, package_version, date_utc);

-- Sync bookkeeping for background poller / 10-day restatements.
CREATE TABLE IF NOT EXISTS sync_state (
    key             TEXT PRIMARY KEY,
    value           TEXT NOT NULL,
    updated_at      TEXT NOT NULL DEFAULT (strftime('%Y-%m-%dT%H:%M:%fZ', 'now'))
);

INSERT OR IGNORE INTO apps (store_id, display_name)
VALUES ('9P0HGKHQ9W19', 'AQEMU');
