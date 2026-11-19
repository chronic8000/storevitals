# StoreVitals — Microsoft sign-in setup

> **Full guide (Entra, card/cost, Partner Center Manager, Sync secrets):**  
> **[ACCOUNT_SETUP.md](ACCOUNT_SETUP.md)**

StoreVitals signs you in with **Hotmail / Outlook.com / Live** or a **work/school** account.  
You do **not** need a client secret for interactive sign-in alone. **Full sync** still needs tenant + client ID + secret — see the full guide.

## Cost (short)

Microsoft Entra ID Free is **not charged**. Creating a tenant from Partner Center is free. An Azure free signup may ask for a debit/credit card for identity verification (temporary ~$1 hold, reversed) — see [ACCOUNT_SETUP.md](ACCOUNT_SETUP.md#cost-is-entra-free-do-i-need-a-debit-card).

## 1. Register a public client app (one-time)

1. Open [App registrations](https://portal.azure.com/#blade/Microsoft_AAD_RegisteredApps/ApplicationsListBlade) while signed in with Microsoft (your Hotmail works for a personal Entra “Default Directory”, or use any directory you can administer).
2. **New registration**
   - Name: `StoreVitals`
   - Supported account types: **Accounts in any organizational directory and personal Microsoft accounts**  
     (Must include personal Microsoft accounts or Hotmail sign-in fails with `unauthorized_client` / “not enabled for consumers”.)
   - Redirect URI platform: **Mobile and desktop applications** (not Web)
   - Redirect URIs (add all):
     - `http://localhost`
     - `http://localhost:8400`
3. After creation, copy **Application (client) ID**.
4. **Authentication**
   - Platform must be **Mobile and desktop applications**.
   - Confirm both localhost URIs above are listed (exact match — no `https`, no trailing slash on `:8400`).
   - Enable **Allow public client flows** → Yes.
   - If browser sign-in still says `redirect_uri` is invalid, use **Use device code** in StoreVitals (no redirect URI needed).
5. If you already registered the app as **single-tenant / org-only**, open the registration → **Manifest** (or Authentication) and set supported account types to include **personal Microsoft accounts**, then Save. In StoreVitals prefer **Personal + work**.
6. No client secret is required for StoreVitals interactive sign-in.

### Partner Center note (work or personal publishers)

Keep your normal Partner Center login — **work/school or** Hotmail / Outlook.com.  
Analytics Sync still needs a **one-time Entra app** with role **Manager**. That is not “switching” your publisher account.

- **Work/school:** you already have Microsoft Entra ID. Associate that org directory if needed, register the app there (you usually do not create a new tenant).  
- **Personal Microsoft account:** Associate or **Create** a free Entra tenant from Partner Center → **Tenants**, then register the app.

Then: **User management → Microsoft Entra applications** → add **StoreVitals** → role **Manager**.  
Sync uses **client credentials** (tenant + client ID + secret). A **401** after a good token almost always means Manager is missing.

Full paths: [ACCOUNT_SETUP.md](ACCOUNT_SETUP.md).

## 2. Paste the client ID into StoreVitals

1. Run StoreVitals.
2. Open **Settings / Sign in**.
3. Paste the Application (client) ID.
4. Account type: **Personal + work (recommended)** (or Personal only for Hotmail-only).
5. Click **Sign in with Microsoft** (or **Use device code**).

Refresh tokens are stored only in **Windows Credential Manager**.

## 3. CLI (optional)

```powershell
# After saving the client ID in Settings once:
.\StoreVitals.exe --signin
.\StoreVitals.exe --signin --device-code
.\StoreVitals.exe --sync 9P0HGKHQ9W19
```
