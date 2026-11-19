# StoreVitals — developer packaging & license gate

Product brand: **StoreVitals** (formerly Store Pulse). CMake target / QML module URI remain `StorePulse` internally; the shipped EXE is `StoreVitals.exe`.

This file is for **developers** building and packaging the app — not end-user account setup. Users: see [`docs/ACCOUNT_SETUP.md`](../docs/ACCOUNT_SETUP.md).

## License gate

| Build | CMake flag | Behavior |
| --- | --- | --- |
| Local / CI unpackaged | `-DDEV_BYPASS_STORE_LICENSE=ON` (default) | Always licensed — do **not** document this in user guides |
| Store / MSIX | `-DDEV_BYPASS_STORE_LICENSE=OFF` | `Windows.Services.Store` `GetAppLicenseAsync` |

Never ship a Store build with the bypass flag enabled.

`Package.appxmanifest` identity (must match Partner Center → Product identity):

| Field | Value |
| --- | --- |
| Name | `30932PhilipSempers.StoreVitals` |
| Publisher | `CN=16318CB3-C262-4B44-BCCF-310B0DDA3950` |
| PublisherDisplayName | `Philip Sempers` |
| Store ID | `9MW0C3Q80S4R` |

## Assets

```powershell
python packaging\generate_store_assets.py
```

Produces `packaging\Assets\*.png` and `branding\StorePulse.ico` (window icon).

## Package

x64 Qt kit: `C:\Qt\6.8.1\msvc2022_64`  
ARM64 Qt kit: `C:\Qt_arm\6.12.0\msvc2022_arm64`

```powershell
# x64 (can be built on ARM64 Windows via x64 toolchain / emulation)
cmake -S . -B build\x64 -G Ninja -DDEV_BYPASS_STORE_LICENSE=OFF -DCMAKE_PREFIX_PATH=C:/Qt/6.8.1/msvc2022_64
cmake --build build\x64 --target StorePulse

# native ARM64 (VsDevCmd -arch=arm64)
cmake -S . -B build\arm64 -G Ninja -DDEV_BYPASS_STORE_LICENSE=OFF -DCMAKE_PREFIX_PATH=C:/Qt_arm/6.12.0/msvc2022_arm64
cmake --build build\arm64 --target StorePulse

.\packaging\package_msix.ps1 `
  -X64Dir .\build\x64 `
  -Arm64Dir .\build\arm64 `
  -QtPrefix C:\Qt\6.8.1\msvc2022_64 `
  -QtArm64Prefix C:\Qt_arm\6.12.0\msvc2022_arm64
```

Output: `packaging\out\` — `StoreVitals_x64.msix`, `StoreVitals_arm64.msix`, `StoreVitals.msixbundle`, `StoreVitals.msixupload`.
