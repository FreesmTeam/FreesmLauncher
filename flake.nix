{
  description = "Prism Launcher fork aimed to provide a free way to play Minecraft.";

  nixConfig = {
    substituters = [
      "https://cache.garnix.io"
    ];
    trusted-public-keys = [
      "cache.garnix.io:CTFPyKSLcx5RMJKfLo5EEPUObbA78b0YQ2DTCJXqr9g="
    ];
  };

  inputs = {
    nixpkgs = {
      url = "github:NixOS/nixpkgs/nixos-unstable";
    };

    nix-filter = {
      url = "github:numtide/nix-filter";
    };

    libnbtplusplus = {
      url = "github:FreesmTeam/libnbtplusplus";
      flake = false;
    };

    flake-compat = {
      url = "github:edolstra/flake-compat";
      flake = false;
    };
  };

  outputs =
    {
      self,
      nixpkgs,
      libnbtplusplus,
      nix-filter,
      ...
    }:
    let
      inherit (nixpkgs) lib;
      systems = lib.systems.flakeExposed;
      forAllSystems = lib.genAttrs systems;
      nixpkgsFor = forAllSystems (system: nixpkgs.legacyPackages.${system});
    in
    {

      formatter = forAllSystems (system: nixpkgsFor.${system}.nixfmt-rfc-style);

      checks = forAllSystems (
        system:
        let
          checks' = nixpkgsFor.${system}.callPackage ./nix/checks.nix { inherit self; };
        in
        lib.filterAttrs (_: lib.isDerivation) checks'
      );

      devShells = forAllSystems (
        system:
        let
          pkgs = nixpkgsFor.${system};
        in
        {
          default = pkgs.mkShell {
            inputsFrom = [
              self.packages.${system}.freesmlauncher-unwrapped
            ];
            buildInputs = [
              pkgs.ccache
              pkgs.ninja
              pkgs.llvmPackages_19.clang-tools
            ];
          };
        }
      );

      overlays = {
        default = final: prev: {
          freesmlauncher-unwrapped = prev.callPackage ./nix/unwrapped.nix {
            inherit
              libnbtplusplus
              nix-filter
              self
              ;
          };

          freesmlauncher = final.callPackage ./nix/wrapper.nix { };
        };
      };

      packages = forAllSystems (
        system:
        let
          pkgs = nixpkgsFor.${system};

          freesmPackages = lib.makeScope pkgs.newScope (final: self.overlays.default final pkgs);

          packages = {
            inherit (freesmPackages) freesmlauncher-unwrapped freesmlauncher;
            default = freesmPackages.freesmlauncher;
          };
        in
        lib.filterAttrs (_: lib.meta.availableOn pkgs.stdenv.hostPlatform) packages

      );

      legacyPackages = forAllSystems (
        system:
        let
          freesmPackages = self.packages.${system};
          legacyPackages = self.legacyPackages.${system};
        in
        {
          freesmlauncher-debug = freesmPackages.freesmlauncher.override {
            freesmlauncher-unwrapped = legacyPackages.freesmlauncher-unwrapped-debug;
          };

          freesmlauncher-unwrapped-debug = freesmPackages.freesmlauncher-unwrapped.overrideAttrs {
            cmakeBuildType = "Debug";
            dontStrip = true;
          };
        }
      );
    };
}
