-- Listing page views / conversions by channel (clickCount matches Insights conversion math).

CREATE TABLE IF NOT EXISTS channel_conversions (
    id                  INTEGER PRIMARY KEY AUTOINCREMENT,
    store_id            TEXT NOT NULL REFERENCES apps(store_id) ON DELETE CASCADE,
    date_utc            TEXT NOT NULL,
    channel_type        TEXT NOT NULL DEFAULT '',
    market              TEXT NOT NULL DEFAULT '',
    device_type         TEXT NOT NULL DEFAULT '',
    click_count         INTEGER NOT NULL DEFAULT 0,
    conversion_count    INTEGER NOT NULL DEFAULT 0,
    ingested_at         TEXT NOT NULL DEFAULT (strftime('%Y-%m-%dT%H:%M:%fZ', 'now')),
    UNIQUE (store_id, date_utc, channel_type, market, device_type)
);

CREATE INDEX IF NOT EXISTS idx_channel_conversions_store_date
    ON channel_conversions(store_id, date_utc);
