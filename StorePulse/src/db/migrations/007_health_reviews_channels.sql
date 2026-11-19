-- Crash device details (failuredetails), richer reviews, channel referrers/campaigns.

CREATE TABLE IF NOT EXISTS failure_details (
    id                  INTEGER PRIMARY KEY AUTOINCREMENT,
    store_id            TEXT NOT NULL REFERENCES apps(store_id) ON DELETE CASCADE,
    failure_hash        TEXT NOT NULL,
    failure_name        TEXT NOT NULL DEFAULT '',
    date_utc            TEXT NOT NULL DEFAULT '',
    cab_id              TEXT NOT NULL DEFAULT '',
    cab_expiration      TEXT NOT NULL DEFAULT '',
    cab_downloadable    INTEGER NOT NULL DEFAULT 0,
    market              TEXT NOT NULL DEFAULT '',
    device_type         TEXT NOT NULL DEFAULT '',
    device_model        TEXT NOT NULL DEFAULT '',
    os_version          TEXT NOT NULL DEFAULT '',
    os_build            TEXT NOT NULL DEFAULT '',
    os_release          TEXT NOT NULL DEFAULT '',
    os_architecture     TEXT NOT NULL DEFAULT '',
    package_version     TEXT NOT NULL DEFAULT '',
    cpu_manufacturer    TEXT NOT NULL DEFAULT '',
    cpu_family          TEXT NOT NULL DEFAULT '',
    cpu_name            TEXT NOT NULL DEFAULT '',
    flight_ring         TEXT NOT NULL DEFAULT '',
    sandbox_id          TEXT NOT NULL DEFAULT '',
    event_count         INTEGER NOT NULL DEFAULT 0,
    stack_trace         TEXT NOT NULL DEFAULT '',
    ingested_at         TEXT NOT NULL DEFAULT (strftime('%Y-%m-%dT%H:%M:%fZ', 'now')),
    UNIQUE (store_id, failure_hash, cab_id, date_utc, device_model, os_build, market)
);

CREATE INDEX IF NOT EXISTS idx_failure_details_store_hash
    ON failure_details(store_id, failure_hash);

CREATE INDEX IF NOT EXISTS idx_failure_details_arch
    ON failure_details(store_id, os_architecture);

-- Reviewer hardware + revised + topic tags (tags JSON array as TEXT).
ALTER TABLE reviews ADD COLUMN device_model TEXT NOT NULL DEFAULT '';
ALTER TABLE reviews ADD COLUMN device_ram_mb INTEGER NOT NULL DEFAULT 0;
ALTER TABLE reviews ADD COLUMN device_storage_gb REAL NOT NULL DEFAULT 0;
ALTER TABLE reviews ADD COLUMN device_screen_resolution TEXT NOT NULL DEFAULT '';
ALTER TABLE reviews ADD COLUMN product_family TEXT NOT NULL DEFAULT '';
ALTER TABLE reviews ADD COLUMN is_revised INTEGER NOT NULL DEFAULT 0;
ALTER TABLE reviews ADD COLUMN is_touch_enabled INTEGER NOT NULL DEFAULT 0;
ALTER TABLE reviews ADD COLUMN topic_tags TEXT NOT NULL DEFAULT '';

-- Channel referrers / campaigns (rebuild unique key).
CREATE TABLE IF NOT EXISTS channel_conversions_v2 (
    id                  INTEGER PRIMARY KEY AUTOINCREMENT,
    store_id            TEXT NOT NULL REFERENCES apps(store_id) ON DELETE CASCADE,
    date_utc            TEXT NOT NULL,
    channel_type        TEXT NOT NULL DEFAULT '',
    market              TEXT NOT NULL DEFAULT '',
    device_type         TEXT NOT NULL DEFAULT '',
    referrer_uri_domain TEXT NOT NULL DEFAULT '',
    custom_campaign_id  TEXT NOT NULL DEFAULT '',
    click_count         INTEGER NOT NULL DEFAULT 0,
    conversion_count    INTEGER NOT NULL DEFAULT 0,
    ingested_at         TEXT NOT NULL DEFAULT (strftime('%Y-%m-%dT%H:%M:%fZ', 'now')),
    UNIQUE (
        store_id, date_utc, channel_type, market, device_type,
        referrer_uri_domain, custom_campaign_id
    )
);

INSERT INTO channel_conversions_v2 (
    store_id, date_utc, channel_type, market, device_type,
    referrer_uri_domain, custom_campaign_id, click_count, conversion_count, ingested_at
)
SELECT
    store_id, date_utc, channel_type, market, device_type,
    '', '', click_count, conversion_count, ingested_at
FROM channel_conversions;

DROP TABLE channel_conversions;
ALTER TABLE channel_conversions_v2 RENAME TO channel_conversions;

CREATE INDEX IF NOT EXISTS idx_channel_conversions_store_date
    ON channel_conversions(store_id, date_utc);
