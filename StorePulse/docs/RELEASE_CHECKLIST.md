# StoreVitals — publisher release checklist

Internal checklist for shipping StoreVitals to the Microsoft Store. Not for end users — they follow [ACCOUNT_SETUP.md](ACCOUNT_SETUP.md).

## Legal / Store listing

Canonical hosted pages (GitHub Pages): see [`../../docs/STORE_LISTING_URLS.md`](../../docs/STORE_LISTING_URLS.md).

- [ ] Privacy policy live: `https://chronic8000.github.io/storevitals/privacy.html`  
- [ ] Website live: `https://chronic8000.github.io/storevitals/`  
- [ ] Support live: `https://chronic8000.github.io/storevitals/support.html`  
- [ ] Partner Center product **privacy policy URL** + website/support fields set on the StoreVitals listing.  
- [ ] Entra app **Branding & properties** (optional): logo, home, terms, privacy — see [../branding/ENTRA_BRANDING.md](../branding/ENTRA_BRANDING.md).

## Build / package

- [ ] Release build with `-DDEV_BYPASS_STORE_LICENSE=OFF`.  
- [ ] MSIX / `.msixupload` from [`packaging/README.md`](../packaging/README.md).  
- [ ] Package.appxmanifest matches Product identity (`30932PhilipSempers.StoreVitals`, Publisher `CN=16318CB3-C262-4B44-BCCF-310B0DDA3950`).  
- [ ] Smoke-test license gate on a Store-sideloaded or certification build (bypass must be off).

## Docs

- [ ] User-facing [ACCOUNT_SETUP.md](ACCOUNT_SETUP.md) has no publisher-only steps.  
- [ ] Version / changelog notes ready for the Store submission.
