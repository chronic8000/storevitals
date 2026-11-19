-- Full Partner Center analytics surface: funnel, installs, usage, ratings, reviews.

CREATE TABLE IF NOT EXISTS funnel_metrics (
    id              INTEGER PRIMARY KEY AUTOINCREMENT,
    store_id        TEXT NOT NULL REFERENCES apps(store_id) ON DELETE CASCADE,
    window_start    TEXT NOT NULL,
    window_end      TEXT NOT NULL,
    metric_type     TEXT NOT NULL,  -- PageView, Acquisition, Install, Usage
    user_count      INTEGER NOT NULL DEFAULT 0,
    market          TEXT NOT NULL DEFAULT '',
    device_type     TEXT NOT NULL DEFAULT '',
    ingested_at     TEXT NOT NULL DEFAULT (strftime('%Y-%m-%dT%H:%M:%fZ', 'now')),
    UNIQUE (store_id, window_start, window_end, metric_type, market, device_type)
);

CREATE INDEX IF NOT EXISTS idx_funnel_store
    ON funnel_metrics(store_id, window_start, window_end);

CREATE TABLE IF NOT EXISTS installs (
    id                          INTEGER PRIMARY KEY AUTOINCREMENT,
    store_id                    TEXT NOT NULL REFERENCES apps(store_id) ON DELETE CASCADE,
    date_utc                    TEXT NOT NULL,
    market                      TEXT NOT NULL DEFAULT '',
    device_type                 TEXT NOT NULL DEFAULT '',
    os_version                  TEXT NOT NULL DEFAULT '',
    package_version             TEXT NOT NULL DEFAULT '',
    successful_install_count    INTEGER NOT NULL DEFAULT 0,
    ingested_at                 TEXT NOT NULL DEFAULT (strftime('%Y-%m-%dT%H:%M:%fZ', 'now')),
    UNIQUE (store_id, date_utc, market, device_type, os_version, package_version)
);

CREATE INDEX IF NOT EXISTS idx_installs_store_date
    ON installs(store_id, date_utc);

CREATE TABLE IF NOT EXISTS usage_daily (
    id                      INTEGER PRIMARY KEY AUTOINCREMENT,
    store_id                TEXT NOT NULL REFERENCES apps(store_id) ON DELETE CASCADE,
    date_utc                TEXT NOT NULL,
    market                  TEXT NOT NULL DEFAULT '',
    device_type             TEXT NOT NULL DEFAULT '',
    package_version         TEXT NOT NULL DEFAULT '',
    daily_session_count     INTEGER NOT NULL DEFAULT 0,
    engagement_minutes      REAL NOT NULL DEFAULT 0,
    daily_active_users      INTEGER NOT NULL DEFAULT 0,
    daily_active_devices    INTEGER NOT NULL DEFAULT 0,
    daily_new_users         INTEGER NOT NULL DEFAULT 0,
    ingested_at             TEXT NOT NULL DEFAULT (strftime('%Y-%m-%dT%H:%M:%fZ', 'now')),
    UNIQUE (store_id, date_utc, market, device_type, package_version)
);

CREATE INDEX IF NOT EXISTS idx_usage_store_date
    ON usage_daily(store_id, date_utc);

CREATE TABLE IF NOT EXISTS ratings_daily (
    id                  INTEGER PRIMARY KEY AUTOINCREMENT,
    store_id            TEXT NOT NULL REFERENCES apps(store_id) ON DELETE CASCADE,
    date_utc            TEXT NOT NULL,
    market              TEXT NOT NULL DEFAULT '',
    device_type         TEXT NOT NULL DEFAULT '',
    os_version          TEXT NOT NULL DEFAULT '',
    average_rating      REAL NOT NULL DEFAULT 0,
    rating_count        INTEGER NOT NULL DEFAULT 0,
    one_star            INTEGER NOT NULL DEFAULT 0,
    two_star            INTEGER NOT NULL DEFAULT 0,
    three_star          INTEGER NOT NULL DEFAULT 0,
    four_star           INTEGER NOT NULL DEFAULT 0,
    five_star           INTEGER NOT NULL DEFAULT 0,
    ingested_at         TEXT NOT NULL DEFAULT (strftime('%Y-%m-%dT%H:%M:%fZ', 'now')),
    UNIQUE (store_id, date_utc, market, device_type, os_version)
);

CREATE TABLE IF NOT EXISTS reviews (
    id                  INTEGER PRIMARY KEY AUTOINCREMENT,
    store_id            TEXT NOT NULL REFERENCES apps(store_id) ON DELETE CASCADE,
    review_id           TEXT NOT NULL,
    date_utc            TEXT NOT NULL DEFAULT '',
    rating              INTEGER NOT NULL DEFAULT 0,
    title               TEXT NOT NULL DEFAULT '',
    review_text         TEXT NOT NULL DEFAULT '',
    market              TEXT NOT NULL DEFAULT '',
    os_version          TEXT NOT NULL DEFAULT '',
    device_type         TEXT NOT NULL DEFAULT '',
    package_version     TEXT NOT NULL DEFAULT '',
    reviewer_name       TEXT NOT NULL DEFAULT '',
    helpful_count       INTEGER NOT NULL DEFAULT 0,
    ingested_at         TEXT NOT NULL DEFAULT (strftime('%Y-%m-%dT%H:%M:%fZ', 'now')),
    UNIQUE (store_id, review_id)
);

CREATE INDEX IF NOT EXISTS idx_reviews_store_date
    ON reviews(store_id, date_utc);
