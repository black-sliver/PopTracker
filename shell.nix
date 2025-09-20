{
  pkgs ? import <nixpkgs> { },
}:
pkgs.mkShell {
  buildInputs = [
    pkgs.zlib
    pkgs.cmake
    pkgs.sdl2-compat
    pkgs.SDL2_image
    pkgs.SDL2_ttf
    pkgs.openssl

    # X11 dependencies
    pkgs.libx11
  ];

  shellHook = ''
    echo "PopTracker build environment loaded"
    echo "Run: make native CONF=RELEASE"
    echo "RUN: ./build/<platform>/poptracker"
  '';
}
