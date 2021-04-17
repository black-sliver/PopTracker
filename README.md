# PopTracker
*Powerful open progress tracker* is a project to offer a universal, scriptable
randomizer tracking solution that it is open source, runs everywhere and
supports auto-tracking.

## Getting started
This is work in progress. Some pre-existing packs work, some do not.

Download a binary release or build from source.

Unpack gamepacks to `EXEDIR/packs`, `HOME/PopTracker/packs`, `Documents/PopTracker/packs` or `CWD/packs` (ZIPs not yet supported).
On Windows it will also find packs installed in EmoTracker.
On macOS EXEDIR is *inside* the app bundle.

Use the Load button in the top left corner to load a pack.

Do not load untrusted packs until some sort of fuzzing is done.

Binary release is Windows 64bit and macOS 64bit only at the moment,
should compile on most unix-like OS (see [Building from source](#building-from-source)).
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

## Building from source
See [BUILD.md](BUILD.md).

## Supported/tested packs
* [evermizer-tracker-package](https://github.com/Cyb3RGER/evermizer-tracker-package)
* [SoM-Open-Mode-Tracker](https://github.com/Cyb3RGER/SoM-Open-Mode-Tracker)
* [iogr_emotracker_apokalysme](https://github.com/Apokalysme/iogr_emotracker_apokalysme) v3.6

*more to be tested*

## Auto-tracking
### SNES
Requires [QUsb2Snes](https://usb2snes.com) (flash cart, emu, snes mini) or [usb2snes](https://github.com/RedGuyyyy/sd2snes/releases) (flash cart only). See their respective documentation.

### Other systems
No work has been done for other systems yet.
