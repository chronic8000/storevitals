-- Partner Center Insights AcqV2 page views (storepartneranalytics v1 daily).
-- These are the overview-card totals (~28K), not classic channel clickCount.

CREATE TABLE IF NOT EXISTS insights_pageviews (
    id              INTEGER PRIMARY KEY AUTOINCREMENT,
    store_id        TEXT NOT NULL REFERENCES apps(store_id) ON DELETE CASCADE,
    date_utc        TEXT NOT NULL,
    pageviews_count INTEGER NOT NULL DEFAULT 0,
    ingested_at     TEXT NOT NULL DEFAULT (strftime('%Y-%m-%dT%H:%M:%fZ', 'now')),
    UNIQUE (store_id, date_utc)
);

CREATE INDEX IF NOT EXISTS idx_insights_pageviews_store_date
    ON insights_pageviews(store_id, date_utc);
