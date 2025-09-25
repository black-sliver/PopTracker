{ pkgs ? import <nixpkgs> {} }:

let
  libX11 =
    if builtins.hasAttr "libx11" pkgs then
      pkgs.libx11
    else
      let
        xorgLibX11 = builtins.tryEval pkgs.xorg.libX11;
      in
        if xorgLibX11.success then
          xorgLibX11.value
        else
          throw "Could not find libX11 (neither pkgs.libx11 nor pkgs.xorg.libX11)"
  ;
in

pkgs.mkShell {
  buildInputs = [
    pkgs.zlib
    pkgs.cmake
    pkgs.sdl2-compat
    pkgs.SDL2_image
    pkgs.SDL2_ttf
    pkgs.openssl
    libX11
  ];

  shellHook = ''
    echo "PopTracker build environment loaded"
    echo "Run: make native CONF=RELEASE"
    echo "RUN: ./build/<platform>/poptracker"
  '';
}
