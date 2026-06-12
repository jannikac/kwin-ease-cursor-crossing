# Quick standalone build: nix-build dist/nixos
# Uses the nixpkgs from your NIX_PATH (on NixOS: your system channel), so the
# plugin links against the same KWin your system runs.
{
  pkgs ? import <nixpkgs> { },
}:
pkgs.kdePackages.callPackage ./package.nix { }
