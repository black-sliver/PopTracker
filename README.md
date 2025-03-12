# PopTracker

*Powerful open progress tracker* is a project to offer a universal, scriptable
randomizer tracking solution that it is open source, runs everywhere and
supports auto-tracking.

## Getting started

This is work in progress. Some pre-existing packs work, some do not.

Download a binary release or build from source.

Drag & drop downloaded packs into the PopTracker window to install them without unpacking.\
Alternatively copy or unpack tracker packs into one of the search paths
`EXEDIR/packs` (not on macOS), `HOME/PopTracker/packs`,
`Documents/PopTracker/packs` or `CWD/packs`.

Use the Load button in the top left corner to load a pack.

Do not load untrusted packs until some sort of fuzzing is done.

Binary releases exist for Windows 64bit, macOS 64bit and Linux x86_64 (see [Download prebuilt exe or app](#download-prebuilt-exe-or-app)),\
should compile on most unix-like OS (see [Building from source](#building-from-source)).

Nix users can install [`poptracker`](https://search.nixos.org/packages?show=poptracker&type=packages&query=poptracker) from nixpkgs.

[WASM](https://wikipedia.org/wiki/WebAssembly) support still needs a lot of work.

Check
[BUILD.md](BUILD.md),
[CONTRIBUTING.md](CONTRIBUTING.md),
[doc/OUTLINE.md](doc/OUTLINE.md) and
[doc/TODO.md](doc/TODO.md)
if you want to join in on the development journey.

Check [doc/PACKS.md](doc/PACKS.md) if you want to write a game pack for this tracker.

Upstream URL is https://github.com/black-sliver/PopTracker/

## Screenshot

![Screenshot](../screenshots/screenshot.png?raw=true "Screenshot")

## Download prebuilt exe, app or AppImage

Head over to [releases](https://github.com/black-sliver/PopTracker/releases)
and unfold "Assets" of the latest release or pre-release to get a Windows exe or macOS app.

For the AppImage, you still need to install `which` and a dialog provider (`zenity`, `kdialog`, `matedialog`, `qarma`
or `xdialog`) to run them.

## Building from source

See [BUILD.md](BUILD.md).

## Supported/tested packs

Join the [Community Discord](https://discord.com/invite/gwThqMCPgK) to find pack
repositories, follow updates and get support.

## Location Color Key

| Color  | Meaning                                                                                                                               |
|--------|---------------------------------------------------------------------------------------------------------------------------------------|
| Red    | This check is not currently accessible.                                                                                               |
| Yellow | This check is not logically accessible, but the location can be reached through alternate methods (e.g. glitches, breaking key logic) |
| Orange | Some (but not all) checks at this location are accessible.                                                                            |
| Green  | All checks at this location are logically accessible.                                                                                 |
| Blue   | The check at this location is visible, but you cannot currently access the check.                                                     |
| Other  | Locations with mixed accessibility checks will have the corresponding colors mixed.                                                   |

Colors can be customized by following instructions on
[the Color Picker page](https://poptracker.github.io/color-picker.html).

Press Ctrl+P to switch between "mixed" and "split" map location colors.

## Auto-tracking

### SNES Games
Requires [SNI](https://github.com/alttpo/sni)
or [QUsb2Snes](https://usb2snes.com) (flash cart, emu, snes mini)
or [usb2snes](https://github.com/RedGuyyyy/sd2snes/releases) (flash cart only).
See their respective documentation.

### PC Games
We do not allow direct access to process memory or sockets from Lua. Instead
[UAT](https://github.com/black-sliver/UAT) can be used to receive "variables".

### Archipelago Multiworld
[Archipelago](https://archipelago.gg) allows connecting to a Multiworld as a
tracker. Click on the grey "AP" to connect to a server if the pack supports it.
See [doc/AUTOTRACKING.md](./doc/AUTOTRACKING.md) for more details.

### Bizhawk Connector
Preview, currently only when setting platform to "n64". See
[doc/AUTOTRACKING.md](./doc/AUTOTRACKING.md#supported-interfaces) for details.

### Other systems
No work has been done for other systems yet.

## Keyboard Shortcuts

| Hotkey    | Alternative      | Function                                                                                                         |
|-----------|------------------|------------------------------------------------------------------------------------------------------------------|
| F1        | n/a              | Show this document.                                                                                              |
| F2        | n/a              | Open broadcast window.                                                                                           |
| F5        | Ctrl + R         | Reload: if the pack did not change, loads an empty state (everything unchecked), otherwise same as force-reload. |
| Ctrl + F5 | Ctrl + Shift + R | Force-reload: reload the pack from disk.                                                                         |
| F11       | Ctrl + H         | Toggle visibility of cleared and inaccessible map locations.                                                     |
| Ctrl + P  | n/a              | Toggle between mixed and split colors for map locations.                                                         |

## Version Numbering

* Major update (X.0.0) may break everything
* Minor update (0.X.0) may change render output (i.e. window captures break)
* Revisions (0.0.X) should only fix bugs and add non-breaking features

## Plug-Ins

Currently, there is no plug-in interface.

If you want to work towards implementing such a system, please check
[PLUGIN LICENSE ADDENDUM.md](PLUGIN%20LICENSE%20ADDENDUM.md)
for licensing considerations.

## User Overrides

Users can override files from packs by creating a folder with the same file
structure as in the pack, named `.../user-override/<pack_uid>` where `...` is
any one of `Documents/PopTracker`, `%home%/PopTracker` or `AppPath`.

## Portable Mode

When creating a file called `portable.txt` next to the program (not macos) or
next to poptracker **inside** the AppBundle (macos-only), the app runs in
portable mode, which changes the default pack folder to be next to the program
(not in home folder) and disables asset and pack overrides from home folder
(only allows overrides from program folder).

## Command Line Arguments

PopTracker supports some command line arguments. Run with `--help` from a terminal
or see [doc/commandline.txt](doc/commandline.txt).
