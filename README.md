# ultra-shared

UI framework and CRT-settings code shared between ultraSID and ultraView.
Mounted as `Source/ultra-shared` in each app; the app's `Source/` folder is on
the include path, so module-internal includes use the `ultra-shared/...` prefix.

## Host contract

The module compiles inside each app's target and includes a small set of
app-provided headers at fixed paths. Each app must supply:

| Header | Required interface |
| --- | --- |
| `Config/FilePaths.h` | `namespace filepaths`: `root` enum (`data`/`user`), `markerFor`, `getUserOverlaysPath`, `getUserCRTMasksPath`, `getUserCRTPresetsPath` |
| `Config/Preferences.h` | `class Preferences : YamlFile` with the app's defaults table (must define every key the shared components read, including `crt/preset`) |
| `Config/Settings.h` | `class Settings : YamlFile` with defaults for `install/stale-copy`, `network/password`, `update/last-check`, `update/last-known-version` (AppInstall, AppUpdater, C64u scanner) |
| `Helpers/Messages.h` | `msg::SettingChanged { section, name }` (settings components, theme selector); developer-mode screenshot messages `ToggleFirstLuma`, `ToggleFirstLumaAll`, `ToggleThumbnail`, `DeleteImage`, `RemoveBorderColor`, `AssignBorderColor { index }` (VIC2 palette) |
| `UI/ui-colors.h` | `COLOR_ROLES` X-macro + `UI::colors` enum (see Theme.cpp) |
| `UI/ui-fonts.h`, `UI/ui-corners.h`, `UI/ui-lines.h`, `UI/ui-paddings.h` | The four role registries (X-macros + `Def`/`fromName`) |

Roles and preference keys referenced by shared code by name (e.g. `crt-label`,
`crt/...`, `overlay/...`, `tv/...`, `webcam/...`) must exist in the host's
registries and defaults table.
