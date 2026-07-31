# ultra-shared

UI framework and CRT-settings code shared between ultraSID and ultraView.
Mounted as `Source/ultra-shared` in each app; the app's `Source/` folder is on
the include path, so module-internal includes use the `ultra-shared/...` prefix.

## Host contract

The module compiles inside each app's target and includes a small set of
app-provided headers at fixed paths. Each app must supply:

| Header | Required interface |
| --- | --- |
| `Config/DataSource.h` | `namespace datasource`: `isPak`, `exists`, `loadText`, `loadData`, `loadImage`, `listFiles`, `listFolders`, `getDevFile`, `setActiveUserOverlay`, `setActiveUserCRTMask` |
| `Config/FilePaths.h` | `namespace filepaths`: `root` enum (`data`/`user`), `markerFor`, `isDeveloperMode`, `getUserOverlaysPath`, `getUserCRTMasksPath` |
| `Config/Preferences.h` | `class Preferences : YamlFile` with the app's defaults table (must define every key the shared components read) |
| `Helpers/Messages.h` | `msg::SettingChanged` broadcast (theme selector) |
| `Helpers/ImageUtils.h` | `imageutils::hintFromFilename` (VIC2 screenshot hints) |
| `UI/GUI_LookAndFeel.h` | LookAndFeel class with `fontPoints ( points, weight )` and `monoFontPoints ( points, weight )` |
| `UI/ui-colors.h` | `COLOR_ROLES` X-macro + `UI::colors` enum (see Theme.cpp) |
| `UI/ui-fonts.h`, `UI/ui-corners.h`, `UI/ui-lines.h`, `UI/ui-paddings.h` | The four role registries (X-macros + `Def`/`fromName`) |

Roles and preference keys referenced by shared code by name (e.g. `crt-label`,
`crt/...`, `overlay/...`, `tv/...`, `webcam/...`) must exist in the host's
registries and defaults table.
