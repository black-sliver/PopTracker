# PopTracker
*Powerful open progress tracker* is a project to offer a universal, scriptable
randomizer tracking solution that it is open source, runs everywhere and
supports auto-tracking.

## Getting started
This is work in progress. Some pre-existing packs work, some do not.

Download a binary release or build from source.

Copy or unpack tracker packs to `EXEDIR/packs`, `HOME/PopTracker/packs`, `Documents/PopTracker/packs` or `CWD/packs`.
On Windows it will also find packs installed in EmoTracker.
On macOS EXEDIR is *inside* the app bundle.

Use the Load button in the top left corner to load a pack.

Do not load untrusted packs until some sort of fuzzing is done.

Binary release is Windows 64bit and macOS 64bit only at the moment (see [Download prebuilt exe or app](#download-prebuilt-exe-or-app)),\
should compile on most unix-like OS (see [Building from source](#building-from-source)).\
[WASM](https://wikipedia.org/wiki/WebAssembly) support still needs a lot of work.

Check
[BUILD.md](BUILD.md),
[CONTRIBUTING.md](CONTRIBUTING.md),
[doc/OUTLINE.md](doc/OUTLINE.md) and
[doc/TODO.md](doc/TODO.md)
if you want to join in on the development journey.

Check [doc/PACKS.md](doc/PACKS.md) if you want to write a gamepack for this tracker.

Upstream URL is https://github.com/black-sliver/PopTracker/

## Screenshot
![Screenshot](../screenshots/screenshot.png?raw=true "Screenshot")

## Download prebuilt exe or app
Head over to [releases](https://github.com/black-sliver/PopTracker/releases)
and unfold "Assets" of the latest release or pre-release.

## Building from source
See [BUILD.md](BUILD.md).

## Supported/tested packs
* [evermizer-tracker-package](https://github.com/Cyb3RGER/evermizer-tracker-package)
* [SoM-Open-Mode-Tracker](https://github.com/Cyb3RGER/SoM-Open-Mode-Tracker)
* [iogr_emotracker_apokalysme](https://github.com/Apokalysme/iogr_emotracker_apokalysme) v3.6
* [Ender Lilies Tracker](https://github.com/lurch9229/ender-lilies-poptracker/tree/main/enderlilies_maptracker_lurch9229)

*more to be tested*

Join the [Community Discord](https://discord.com/invite/gwThqMCPgK) to find pack
repositories, follow updates and get support.

## Location Color Key

#### Red
This check is not currently accessible.

#### Yellow
This check is not logically accessible, but the location can be reached through
alternate methods (eg. glitches, breaking key logic)

#### Orange
Some (but not all) checks at this location are accessible.

#### Green
All checks at this location are logically accessible.

#### Blue
The check at this location is visible, but you cannot currently access the check.


## Auto-tracking
### SNES Games
Requires [SNI](https://github.com/alttpo/sni)
or [QUsb2Snes](https://usb2snes.com) (flash cart, emu, snes mini)
or [usb2snes](https://github.com/RedGuyyyy/sd2snes/releases) (flash cart only).
See their respective documentation.

### PC Games
We do not allow direct access to process memory or sockets from Lua. Instead
[UAT](https://github.com/black-sliver/UAT) can be used to recieve "variables".

### Archipelago Multiworld
[Archipelago](https://archipelago.gg) allows connecting to a Multiworld as a
tracker. Click on the grey "AP" to connect to a server if the pack supports it.
See [doc/AUTOTRACKING.md](./doc/AUTOTRACKING.md) for more details.

### Other systems
No work has been done for other systems yet.

## Version Numbering
* Major update (X.0.0) may break everything
* Minor update (0.X.0) may change render output (i.e. window captures break)
* Revisions (0.0.X) should only fix bugs and add non-breaking features
