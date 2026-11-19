# Partner Center → Entra link (short checklist)

> Full walkthrough (work vs personal, cost, app registration, secrets):  
> **[ACCOUNT_SETUP.md](ACCOUNT_SETUP.md)**

Your Partner Center login stays the same — **work/school or** Hotmail / Outlook.com.  
Sync still needs an Entra app once, because Microsoft’s analytics API is app-registration based.

- **Work/school:** you almost certainly **already have** Microsoft Entra ID. Associate that org directory (if needed), register the app there, skip creating a new tenant.  
- **Personal Microsoft account:** create or associate a directory once (Partner Center → Tenants is the usual path).

## Cost

- **Entra ID Free is not billed.**  
- Work tenants: no new signup for this flow.  
- Personal: prefer **Partner Center → Tenants → Create Microsoft Entra ID** (no additional charge). Azure free signup may ask for a card for verification only.

## Checklist

### 1) Partner Center → Account settings → **Tenants**
- **Associate** the Entra directory where you will register StoreVitals  
  (work: your company directory; personal: the directory you created or already manage).  
- Or **Create** a free Microsoft Entra ID tenant if you use a personal account and none is linked.

### 2) Sign in when prompted with a **directory admin**
- Work: an Entra admin for that tenant, if your own account cannot complete the link.  
- Personal: `someone@….onmicrosoft.com` or the Microsoft account that owns the directory.  
- This does **not** replace your usual Partner Center login for publishing.

### 3) User management → **Microsoft Entra applications**
- Add **StoreVitals** (client ID from Azure / StoreVitals Settings).
- Role: **Manager**.
- Save.

### 4) StoreVitals
- Settings → API credentials filled (tenant + client ID + secret).
- Dashboard → **Full sync**.

## How you know it worked
Logs should show a Dev Center token (`aud` contains `manage.devcenter.microsoft.com`) and then acquisitions without `401`.

## If User management still says “sign in with Entra”
You have not finished **Tenants** association yet. Stay on **Tenants** until a directory is listed as associated, then use **Sign in with Microsoft Entra ID**.
