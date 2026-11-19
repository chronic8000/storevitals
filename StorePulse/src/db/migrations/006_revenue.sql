-- Gross sales (USD) from Partner Center acquisitions / add-on acquisitions.
-- Matches Insights "Gross sales" (not net payouts after refunds).

ALTER TABLE acquisitions ADD COLUMN purchase_price_usd REAL NOT NULL DEFAULT 0;
ALTER TABLE acquisitions ADD COLUMN purchase_tax_usd REAL NOT NULL DEFAULT 0;

CREATE TABLE IF NOT EXISTS revenue_daily (
    id                  INTEGER PRIMARY KEY AUTOINCREMENT,
    store_id            TEXT NOT NULL REFERENCES apps(store_id) ON DELETE CASCADE,
    date_utc            TEXT NOT NULL,
    source              TEXT NOT NULL DEFAULT 'app'
                            CHECK (source IN ('app', 'iap')),
    product_id          TEXT NOT NULL DEFAULT '',
    product_name        TEXT NOT NULL DEFAULT '',
    acquisition_type    TEXT NOT NULL DEFAULT '',
    market              TEXT NOT NULL DEFAULT '',
    quantity            INTEGER NOT NULL DEFAULT 0,
    purchase_price_usd  REAL NOT NULL DEFAULT 0,
    purchase_tax_usd    REAL NOT NULL DEFAULT 0,
    ingested_at         TEXT NOT NULL DEFAULT (strftime('%Y-%m-%dT%H:%M:%fZ', 'now')),
    UNIQUE (store_id, date_utc, source, product_id, acquisition_type, market)
);

CREATE INDEX IF NOT EXISTS idx_revenue_daily_store_date
    ON revenue_daily(store_id, date_utc);
