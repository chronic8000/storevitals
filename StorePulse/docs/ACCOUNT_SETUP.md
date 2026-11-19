# StoreVitals — complete account setup

This is the end-to-end guide for connecting **Partner Center** to **Microsoft Entra ID** so StoreVitals can sync Store analytics.

You keep the same account you already use for publishing. Sync needs a one-time Entra **app registration** (tenant ID + client ID + client secret) linked into Partner Center as **Manager**.

Related shorter notes:

- [AUTH_SETUP.md](AUTH_SETUP.md) — interactive sign-in registration only  
- [PARTNER_CENTER_MSA_LINK.md](PARTNER_CENTER_MSA_LINK.md) — short Tenants → Manager checklist  

---

## Which account do you use?

| Partner Center login | Do you already have Entra ID? | What you do |
| --- | --- | --- |
| **Work or school** (`you@company.com`) | **Usually yes** — that account already lives in your organization’s Microsoft Entra tenant (Microsoft 365 / Azure AD). | Associate that **existing** directory with Partner Center (if not already). Register the StoreVitals app **in that tenant**. You typically do **not** create a new tenant or an Azure free account. You may need an Entra admin (or permission to register apps). |
| **Personal Microsoft account** (Hotmail, Outlook.com, Live, Xbox) | **Not by default** for Partner Center API. | Create or associate an Entra directory once (often from Partner Center → Tenants), then register the app there. |

Both paths end the same way: Entra app → Partner Center role **Manager** → paste credentials into StoreVitals.

---

## Cost: is Entra free? Do I need a debit card?

**Microsoft Entra ID Free has no product charge** for the directory or the app registration StoreVitals needs.

| Path | Card required? | Notes |
| --- | --- | --- |
| **Already on a work/school (Entra) tenant** | **No** for this setup | Use the org directory you already have. No new Azure signup. |
| **Partner Center → Tenants → Create Microsoft Entra ID** | Usually **no** | Common for personal Microsoft account publishers. Documented as **no additional charge**. |
| **Azure free account** / new billing account | **Yes** — credit or debit (non-prepaid) | Only if you are creating a directory outside Partner Center. Identity verification only; Entra Free is **not charged**. Temporary **~$1** hold may appear and reverse. |
| **Azure for Students** | No card | University email verification instead. |

**Practical advice**

1. Work account? Start from your existing tenant — skip Azure free signup.  
2. Personal account? Prefer **Partner Center → Account settings → Tenants** to create/associate.  
3. If Azure signup asks for a card: verification only — do not enable paid Azure services you do not need. Avoid prepaid cards; confirm current rules for your country on Microsoft’s signup page.

Official references:

- [Microsoft Entra ID Free](https://learn.microsoft.com/en-us/azure/cost-management-billing/manage/microsoft-entra-id-free)  
- [Azure free account FAQ](https://azure.microsoft.com/en-us/pricing/purchase-options/azure-account)  
- [Create Entra tenant from Partner Center](https://learn.microsoft.com/en-us/windows/apps/publish/partner-center/create-new-azure-ad-tenant)  

---

## Big picture

```
Partner Center login
  (work/school ── already in Entra)
  (or Hotmail / Outlook.com / personal MSA)
        │
        │  associate directory if needed (one-time)
        ▼
Microsoft Entra ID tenant  ──►  App registration "StoreVitals"
        │                              │
        │                              ├─ public client (browser / device-code sign-in)
        │                              └─ client secret (Dev Center analytics Sync)
        ▼
Partner Center → User management → Microsoft Entra applications
        │
        │  role: Manager
        ▼
StoreVitals Settings → tenant + client ID + secret → Full sync
```

---

## Checklist (do in order)

1. [ ] Have a Partner Center publisher account (same work or personal account you publish with).  
2. [ ] Ensure an Entra directory is linked in Partner Center → **Tenants** (use your **existing** work tenant, or create/associate one if you use a personal Microsoft account).  
3. [ ] Register the StoreVitals Entra application in that directory (account types + redirects + public client).  
4. [ ] Create a **client secret** for Sync.  
5. [ ] Optional: add a logo and name to your Entra app.  
6. [ ] In Partner Center, add that Entra app with role **Manager**.  
7. [ ] Paste Directory (tenant) ID, Application (client) ID, and secret into StoreVitals → Settings.  
8. [ ] Sign in interactively (optional but useful) + run **Full sync**.  

---

## Step 1 — Partner Center tenant (Entra directory)

1. Sign in to [Partner Center](https://partner.microsoft.com/dashboard) with the account you publish with (work/school **or** personal Microsoft account).  
2. Gear icon → **Account settings** → **Tenants**  
   Direct link: <https://partner.microsoft.com/dashboard/account/v3/tenant>  
3. Choose the path that matches you:
   - **Work / school:** **Associate Microsoft Entra ID** (or confirm your org directory is already listed). Sign in with an account that can admin that tenant if prompted. You rarely need **Create**.  
   - **Personal Microsoft account:** **Associate** a directory you already manage, **or** **Create Microsoft Entra ID** (free from this page).  
4. When prompted for directory admin, that may be your work Global/Application admin, `you@….onmicrosoft.com`, or the Microsoft account that owns a personal directory — this is only for linking, not a new Partner Center login.  
5. Stay on **Tenants** until a directory is listed as associated.

If **User management** still says “sign in with Entra”, the tenant association is not finished yet.

**Work-account tip:** If app registration is blocked (“You do not have permission”), ask an Entra admin to either register the app for you or allow users to register applications. You still need Partner Center **Manager** on that app afterward.

---

## Step 2 — Register the StoreVitals app in Entra

Open [App registrations](https://portal.azure.com/#blade/Microsoft_AAD_RegisteredApps/ApplicationsListBlade) while signed into the **same directory** you associated above (for work accounts: your company directory in the Azure / Entra portal directory picker).

### 2.1 New registration

| Field | Value |
| --- | --- |
| **Name** | `StoreVitals` |
| **Supported account types** | **Accounts in any organizational directory and personal Microsoft accounts** (recommended). Work-only orgs may use **single tenant** if everyone signs in with that directory; include personal Microsoft accounts if you also use Hotmail for anything in StoreVitals. |
| **Redirect URI platform** | **Mobile and desktop applications** (not Web) |
| **Redirect URIs** | `http://localhost` **and** `http://localhost:8400` |

Exact match matters: no `https`, no trailing slash on `:8400`.

After creation, copy:

- **Application (client) ID**  
- **Directory (tenant) ID** (Overview page)

### 2.2 Authentication

1. Open the app → **Authentication**.  
2. Platform = **Mobile and desktop applications**.  
3. Confirm both localhost redirect URIs.  
4. **Allow public client flows** → **Yes** → Save.  

If browser sign-in later fails with `redirect_uri`, use **Use device code** in StoreVitals (no redirect needed).

### 2.3 Client secret (required for Full sync)

Analytics Sync uses **client credentials**, not your password.

1. App registration → **Certificates & secrets**.  
2. **New client secret** → description e.g. `StoreVitals sync` → expiry you prefer (24 months is fine).  
3. Copy the **Value** immediately (shown once). This is the secret StoreVitals stores in Windows Credential Manager.  

Do **not** commit the secret to git or paste it into chat logs.

### 2.4 Branding & properties (optional)

You can leave Branding & properties blank. The sign-in screen you see is for your own Entra app, so terms and privacy links are optional.

If you want the app to look nicer in Azure / Entra:

| Field | Suggestion |
| --- | --- |
| Logo | 215×215 PNG |
| Name | `StoreVitals` (or any label you prefer) |
| Home / terms / privacy URLs | Optional — leave blank if you do not have pages |
| Publisher display name | Optional |

### 2.5 What you do **not** need

- No client secret for **interactive** sign-in alone.  
- No Graph / custom API permissions for the Dev Center analytics path StoreVitals uses (`resource=https://manage.devcenter.microsoft.com`).  
- Partner Center **Manager** on the app (next step) is what authorizes analytics — not a Graph admin consent checkbox.

---

## Step 3 — Grant the app Manager in Partner Center

This is the step that fixes most **401** Sync failures after a token is issued.

1. Partner Center → **Account settings** → **User management**  
   Direct: <https://partner.microsoft.com/dashboard/account/v3/usermanagement>  
2. Open the **Microsoft Entra applications** tab.  
3. **Add** / select the StoreVitals app (same client ID).  
4. Role: **Manager**.  
5. Save.  

Optional: open the app in that list and confirm Tenant ID / Client ID match Azure; you can also **Add new key** from Partner Center if you prefer generating the secret there instead of Certificates & secrets.

Microsoft docs: [Add and manage Microsoft Entra applications](https://learn.microsoft.com/en-us/partner-center/account-settings/manage-aad-apps).

---

## Step 4 — Configure StoreVitals

Run StoreVitals → **Settings / Sign in**.

### Interactive sign-in (optional identity)

1. Paste **Application (client) ID**.  
2. Account type: **Personal + work (recommended)** — covers both Hotmail and work/school. Use **Personal only** or **Work only** only if you know you will never mix them.  
3. **Sign in with Microsoft** or **Use device code** (same account you use for Partner Center is fine).  

Refresh tokens go to **Windows Credential Manager** only (never SQLite).

### API credentials (required for Sync)

| StoreVitals field | Where to copy from |
| --- | --- |
| **Directory (tenant) ID** | Entra app → Overview → Directory (tenant) ID |
| **Application (client) ID for API** | Same Application (client) ID (usually) |
| **Client secret** | Certificates & secrets → Value (or Partner Center key) |

Click **Save API settings**. Status should show **API ready**.

Secrets live under Credential Manager target `StoreVitals/EntraClientSecret` (legacy `StorePulse/…` is still read as a fallback).

---

## Step 5 — First sync

1. Dashboard → select your app (or **Add app** with its Store ID, e.g. `9P0HGKHQ9W19` for AQEMU VM Manager).  
2. **Full sync**.  
3. Watch the top progress bar and console / status line (`funnel`, `installs`, `ratings`, …).  

**Success looks like**

- Token audience contains `manage.devcenter.microsoft.com`  
- Log: `Full sync finished for <StoreId> ok=1`  
- Button returns to **Full sync** (not stuck on Syncing…)  
- Metrics / charts populate  

**CLI (optional)**

```powershell
.\StoreVitals.exe --signin
.\StoreVitals.exe --signin --device-code
.\StoreVitals.exe --sync 9P0HGKHQ9W19
```

`--sync` always uses Dev Center client credentials (tenant + client ID + secret).

---

## Settings reference (everything we configured)

### Entra app registration

| Setting | Value |
| --- | --- |
| Name | StoreVitals |
| Account types | Org directories **and** personal Microsoft accounts |
| Platform | Mobile and desktop applications |
| Redirect URIs | `http://localhost`, `http://localhost:8400` |
| Allow public client flows | Yes |
| Client secret | Yes (for Sync) |
| Branding | Optional (logo / name) |

### Partner Center

| Setting | Value |
| --- | --- |
| Publisher login | Your work or personal Microsoft account (unchanged) |
| Tenants | Your org Entra directory **or** a created/associated personal directory |
| Entra application role | **Manager** |

### StoreVitals app

| Setting | Storage |
| --- | --- |
| Interactive client ID | Local app config |
| Tenant ID + API client ID | Local app config |
| Client secret | Windows Credential Manager |
| Sign-in refresh token | Windows Credential Manager |
| Analytics data | Local SQLite |

---

## Troubleshooting

| Symptom | Likely cause | Fix |
| --- | --- | --- |
| `unauthorized_client` / not enabled for consumers | App is org-only but you signed in with a personal Microsoft account | Set supported account types to include **personal Microsoft accounts**, or sign in with work/school |
| Cannot register app in Azure | Org policy / missing Entra role | Ask an admin, or use an account with Application Administrator (or similar) |
| `redirect_uri` mismatch | Wrong platform or URI | Mobile/desktop + exact `http://localhost` / `:8400`; or use device code |
| Token OK but Sync **401** | Entra app not Manager in Partner Center | User management → Microsoft Entra applications → Manager |
| “Sign in with Entra” on User management | Tenant not associated | Finish **Tenants** first |
| API incomplete in Settings | Missing tenant, client ID, or secret | Save all three; secret must be stored (not only pasted once and discarded without Save) |
| Secret expired | Certificates & secrets expiry | Create a new secret; paste into StoreVitals; Clear old if needed |
| Stuck “Syncing…” | Older build | Update to a build that clears busy before `syncFinished` |

---

## Security notes

- Never commit tenant secrets, `.env` files with secrets, or Credential Manager exports.  
- Prefer rotating the client secret if it was ever shared or checked into a log.  
- StoreVitals does not store your Microsoft password.
