{
  description = "Prism Launcher fork aimed to provide a free way to play Minecraft.";

  nixConfig = {
    substituters = [
      "https://cache.nixos.org"
      "https://cache.garnix.io"
      "https://freesmlauncher.cachix.org"
    ];
    trusted-public-keys = [
      "cache.nixos.org-1:6NCHdD59X431o0gWypbMrAURkbJ16ZPMQFGspcDShjY="
      "cache.garnix.io:CTFPyKSLcx5RMJKfLo5EEPUObbA78b0YQ2DTCJXqr9g="
      "freesmlauncher.cachix.org-1:Jcp5Q9wiLL+EDv8Mh7c6L9xGk+lXr7/otpKxMOuBuDs="
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

  outputs = {
    self,
    nixpkgs,
    libnbtplusplus,
    nix-filter,
    ...
  }: let
    inherit (nixpkgs) lib;
    systems = lib.systems.flakeExposed;
    forAllSystems = lib.genAttrs systems;
    nixpkgsFor = forAllSystems (system: nixpkgs.legacyPackages.${system});
  in {
    formatter = forAllSystems (system: nixpkgsFor.${system}.alejandra);
    devShells = forAllSystems (
      system: let
        pkgs = nixpkgsFor.${system};
      in {
        default = pkgs.mkShell {
          inputsFrom = [
            self.packages.${system}.freesmlauncher-unwrapped
          ];
          buildInputs = [
            pkgs.ccache
            pkgs.ninja
          ];
          shellHook = ''
            # https://discourse.nixos.org/t/qt-development-environment-on-a-flake-system/23707/5
            setQtEnvironment=$(mktemp)
            random=$(openssl rand -base64 20 | sed "s/[^a-zA-Z0-9]//g")
            makeWrapper "$(type -p sh)" "$setQtEnvironment" "''${qtWrapperArgs[@]}" --argv0 "$random"
            sed "/$random/d" -i "$setQtEnvironment"
            source "$setQtEnvironment"
          '';
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

        freesmlauncher = final.callPackage ./nix/wrapper.nix {};
      };
    };

    packages = forAllSystems (
      system: let
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
      system: let
        freesmPackages = self.packages.${system};
        legacyPackages = self.legacyPackages.${system};
      in {
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
