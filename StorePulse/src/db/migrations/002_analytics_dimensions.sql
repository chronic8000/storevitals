-- Dimensional analytics: OS version on acquisitions; package name for arch inference.

PRAGMA foreign_keys = OFF;

CREATE TABLE acquisitions_v2 (
    id                  INTEGER PRIMARY KEY AUTOINCREMENT,
    store_id            TEXT NOT NULL REFERENCES apps(store_id) ON DELETE CASCADE,
    date_utc            TEXT NOT NULL,
    granularity         TEXT NOT NULL DEFAULT 'daily'
                            CHECK (granularity IN ('hourly', 'daily')),
    acquisition_type    TEXT NOT NULL DEFAULT '',
    market              TEXT NOT NULL DEFAULT '',
    device_type         TEXT NOT NULL DEFAULT '',
    os_version          TEXT NOT NULL DEFAULT '',
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
        device_type, os_version, age_group, gender, store_client, channel, campaign_id, source
    )
);

INSERT INTO acquisitions_v2 (
    id, store_id, date_utc, granularity, acquisition_type, market, device_type,
    os_version, age_group, gender, store_client, channel, campaign_id, quantity,
    source, ingested_at, restated_at
)
SELECT
    id, store_id, date_utc, granularity, acquisition_type, market, device_type,
    '', age_group, gender, store_client, channel, campaign_id, quantity,
    source, ingested_at, restated_at
FROM acquisitions;

DROP TABLE acquisitions;
ALTER TABLE acquisitions_v2 RENAME TO acquisitions;

CREATE INDEX IF NOT EXISTS idx_acquisitions_store_date
    ON acquisitions(store_id, date_utc);

CREATE INDEX IF NOT EXISTS idx_acquisitions_restatement
    ON acquisitions(store_id, date_utc, granularity);

CREATE TABLE failure_hits_v2 (
    id                  INTEGER PRIMARY KEY AUTOINCREMENT,
    store_id            TEXT NOT NULL REFERENCES apps(store_id) ON DELETE CASCADE,
    date_utc            TEXT NOT NULL,
    failure_name        TEXT NOT NULL DEFAULT '',
    failure_hash        TEXT NOT NULL DEFAULT '',
    failure_stack_hash  TEXT NOT NULL DEFAULT '',
    os_version          TEXT NOT NULL DEFAULT '',
    os_architecture     TEXT NOT NULL DEFAULT '',
    package_name        TEXT NOT NULL DEFAULT '',
    package_version     TEXT NOT NULL DEFAULT '',
    event_type          TEXT NOT NULL DEFAULT '',
    event_count         INTEGER NOT NULL DEFAULT 0,
    cab_id              TEXT NOT NULL DEFAULT '',
    stack_trace         TEXT,
    first_seen_at       TEXT,
    last_seen_at        TEXT,
    ingested_at         TEXT NOT NULL DEFAULT (strftime('%Y-%m-%dT%H:%M:%fZ', 'now')),
    restated_at         TEXT,
    UNIQUE (
        store_id, date_utc, failure_hash, failure_stack_hash,
        os_version, os_architecture, package_name, package_version, event_type, cab_id
    )
);

INSERT INTO failure_hits_v2 (
    id, store_id, date_utc, failure_name, failure_hash, failure_stack_hash,
    os_version, os_architecture, package_name, package_version, event_type,
    event_count, cab_id, stack_trace, first_seen_at, last_seen_at, ingested_at, restated_at
)
SELECT
    id, store_id, date_utc, failure_name, failure_hash, failure_stack_hash,
    os_version, os_architecture, '', package_version, event_type,
    event_count, cab_id, stack_trace, first_seen_at, last_seen_at, ingested_at, restated_at
FROM failure_hits;

DROP TABLE failure_hits;
ALTER TABLE failure_hits_v2 RENAME TO failure_hits;

CREATE INDEX IF NOT EXISTS idx_failure_hits_store_date
    ON failure_hits(store_id, date_utc);

CREATE INDEX IF NOT EXISTS idx_failure_hits_spike
    ON failure_hits(store_id, package_version, date_utc);

CREATE INDEX IF NOT EXISTS idx_failure_hits_arch
    ON failure_hits(store_id, os_architecture, date_utc);

PRAGMA foreign_keys = ON;
