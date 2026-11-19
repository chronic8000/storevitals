# Project Directive: Build "Store Pulse" (Native Windows Partner Center Analytics App)

Build **Store Pulse**, a fast, native Windows 11 desktop app (compiled natively for both x64 and ARM64 for deployment on Snapdragon X Elite and Intel/AMD rigs) that pulls Microsoft Store / Partner Center analytics into a local SQLite database, providing a live-feeling dashboard that outperforms the official Partner Center web portal.

## 1. TECH STACK & ARCHITECTURE
- **Language & Framework:** C++20 with Qt 6 (Qt Quick/QML for UI, Qt Charts / QtGraphs for data visualization).
- **Local Database:** SQLite3 compiled in **WAL mode** (Write-Ahead Logging) acting as the primary local cache and historical store. **All UI views and charts must read exclusively from SQLite**, never directly from the network API.
- **Background Poller:** A resilient background polling loop (running in the same process initially, designed with thread isolation) that respects HTTP 429 backoff headers, handles retry queues, and updates the SQLite cache while the UI is closed.
- **Compilation Targets:** Native MSVC / CMake toolchains for `x86_64` and `ARM64`.

## 2. AUTHENTICATION & SECURITY

### Primary: Sign in with Microsoft (Hotmail / MSA + work/school)
Most indie publishers (including Hotmail / Outlook.com Partner Center logins) should **not** need to paste client secrets day-to-day.

- **Interactive OAuth2** authorization-code + **PKCE** (public client), with **device-code** fallback for tray / headless prompts.
- Authority audience defaults to **`common`** (personal Microsoft accounts **and** work/school). Also support `consumers` (Hotmail-only) and `organizations` (work-only).
- Resource/scope targets Dev Center analytics: `https://manage.devcenter.microsoft.com` (+ `offline_access`).
- Cache access tokens **in memory** (~60 minutes); store **refresh tokens exclusively** in Windows Credential Manager. Never write secrets, tokens, or PII to SQLite, logs, or plain-text config.
- CLI: `StorePulse --signin` (browser) or `StorePulse --signin --device-code`.
- UI: **Settings / Sign in** page for Application (client) ID, audience (common/consumers/organizations), browser or device-code sign-in, and sign-out. Setup guide: `StorePulse/docs/AUTH_SETUP.md`.

### Advanced (optional): Entra client credentials
For headless servers / automation, keep support for Partner Center *Account settings > Users > Microsoft Entra applications* (**Manager** role) using client-credentials against `https://login.microsoftonline.com/{tenantId}/oauth2/token` with `resource=https://manage.devcenter.microsoft.com`. Store `tenantId`/`clientId` in settings; **client secret only** in Credential Manager; surface secret expiration warnings.

### Microsoft API note
The Store analytics REST API is still gated by Microsoft identity. Interactive sign-in is the product UX for Hotmail and corporate users. Some accounts may still need a one-time Partner Center identity/app association step before analytics calls succeed — Store Pulse should guide that in Settings rather than requiring client-secret-first setup.

## 3. PARTNER CENTER API INTEGRATION
Implement robust API clients covering the following core endpoints:
1. **Analytics REST Endpoints (`v1.0/my/analytics/`):**
   - `appacquisitions` & `inappacquisitions`
   - `installs`
   - `funnel` (page views, acquisitions, channel/campaign conversions)
   - `usagedaily` / `usagemonthly` (DAU, MAU, session minutes)
   - `failurehits` (crashes, stack traces, CAB file metadata, `osArchitecture`)
   - `ratings` and `reviews` (review text, device model, RAM, storage, package version)
2. **Scheduled Reports API (Consumer Insights v1.1):**
   - Base path: `manage.devcenter.microsoft.com/consumer/insights/v1.1/`
   - Poll hourly scheduled reports (`TIMESPAN LAST_72_HOURS`, `AGGREGATED Hourly`, 1-hour recurrence) for high-resolution acquisitions and installs.
3. **Data Ingestion & Restatements:**
   - Handle pagination (`top`, `skip`, `@nextLink`).
   - Implement **10-day restatement upserts**: Re-pull and reconcile the last 10 days of acquisition data on every background sync run to handle Microsoft's delayed reporting revisions.

## 4. CORE FEATURES (PARITY + "BETTER THAN WEB")
- **Multi-App Cockpit:** Support multiple apps in the publisher account (with initial test focus on apps like AQEMU, Store ID `9P0HGKHQ9W19`).
- **Release Timeline Overlays:** Correlate GitHub release tags and version first-seen dates directly onto acquisition, crash, and adoption charts.
- **Crash Spikes & Windows Toast Alerts:** Poll `failurehits` hourly for recent releases and trigger native Windows 11 toast notifications via WinRT/Qt when crash spikes exceed thresholds.
- **Review Inbox & Sentiment:** Display user reviews with metadata (helpful counts, device RAM, package version) and generate smart drafted replies (note: actual posting requires manual submission in Partner Center until API support lands).
- **Infinite History:** Retain telemetry past Microsoft's rigid 30- and 90-day API deletion windows using local SQLite storage.
- **Data Export:** Fast export options for SQLite data to CSV and Parquet formats.
- **System Tray Integration:** Minimize-to-tray capability with a compact pulse widget/status summary.

## 5. PROJECT STRUCTURE & CODE ORGANIZATION
```text
StorePulse/
├── CMakeLists.txt
├── src/
│   ├── core/           # Config, CredentialManager (Win32 Cred API), Logger
│   ├── api/            # EntraAuthClient, PartnerCenterClient, ScheduledReports
│   ├── db/             # SQLiteDB, WAL setup, migration scripts, repositories
│   ├── background/     # PollingEngine, background sync worker, rate-limiter (429 handling)
│   └── ui/             # QML views, controllers, viewmodels, tray integration
└── tests/              # Unit tests for API parsers, SQL upserts, and restatement logic
```

## Bootstrap priority
1. CMake configuration
2. Secure Windows Credential Manager wrapper
3. Initial SQLite WAL database schema with upsert logic for acquisitions and failure hits

## Test focus app
- AQEMU — Store ID `9P0HGKHQ9W19`
