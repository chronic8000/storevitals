# StoreVitals

Native Windows 11 Partner Center analytics app (C++20 / Qt 6). Formerly developed as Store Pulse. See [`../SPEC.md`](../SPEC.md) for the product directive.

## Account setup (start here)

**[docs/ACCOUNT_SETUP.md](docs/ACCOUNT_SETUP.md)** — Microsoft account + Entra ID, Partner Center Manager link, debit/credit card notes (Entra Free is not charged), every app registration setting, and StoreVitals Settings fields for Sync.

Shorter companions:

- [docs/AUTH_SETUP.md](docs/AUTH_SETUP.md) — interactive sign-in client ID  
- [docs/PARTNER_CENTER_MSA_LINK.md](docs/PARTNER_CENTER_MSA_LINK.md) — Hotmail → Entra checklist  
- [branding/ENTRA_BRANDING.md](branding/ENTRA_BRANDING.md) — Entra logo / ToS / privacy URLs  

## Bootstrap status

- CMake presets for **x64** and **ARM64** (Ninja + VS)
- **Auth (primary):** interactive Microsoft sign-in (Hotmail/MSA + work/school) via OAuth2 PKCE / device code; refresh tokens in Credential Manager
- **Auth (Sync):** Entra client-credentials (tenant + client ID + secret) for Dev Center analytics
- **UI:** Settings / Sign-in + Dashboard (Qt Quick), sync progress bar, review inbox drawer
- SQLite **WAL** database, migrations, acquisition + failure-hit **upsert** repositories
- MSIX packaging under `packaging/`
- Tests: `DbUpserts`, `AnalyticsParsers`, `ApiClient`

## Configure & build

```powershell
cd StorePulse
cmake --preset windows-x64-vs
cmake --build --preset windows-x64-vs
ctest --test-dir build/vs-x64 -C RelWithDebInfo --output-on-failure
```

Requires Qt 6.5+ (`Core`, `Gui`, `Network`, `Sql`, `Quick`, `QuickControls2`, `Test`) and MSVC.

Set `CMAKE_PREFIX_PATH` to your Qt kit if CMake cannot find it, e.g.:

```powershell
cmake --preset windows-x64-vs -DCMAKE_PREFIX_PATH="C:/Qt/6.8.1/msvc2022_64"
```

Developer / Store packaging (license gate, MSIX, `DEV_BYPASS_STORE_LICENSE`): see [`packaging/README.md`](packaging/README.md).  
Publisher Store submission checklist (privacy policy, bypass off, etc.): [`docs/RELEASE_CHECKLIST.md`](docs/RELEASE_CHECKLIST.md).
