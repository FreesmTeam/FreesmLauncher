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
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    libnbtplusplus = {
      url = "github:FreesmTeam/libnbtplusplus";
      flake = false;
    };
  };

  outputs = {
    self,
    nixpkgs,
    libnbtplusplus,
    ...
  }: let
    systems = [
      "x86_64-linux"
      "aarch64-linux"
      "x86_64-darwin"
      "aarch64-darwin"
    ];

    forEachSystem = nixpkgs.lib.genAttrs systems;
  in {
    overlays.default = final: prev: {
      freesmlauncher-unwrapped = final.callPackage ./nix/unwrapped.nix {
        inherit libnbtplusplus self;
      };

      freesmlauncher = final.callPackage ./nix/wrapper.nix;
    };

    packages = forEachSystem (system: let
      pkgs = import nixpkgs {inherit system;};

      freesmlauncher-unwrapped = pkgs.callPackage ./nix/unwrapped.nix {
        inherit libnbtplusplus self;
      };

      freesmlauncher = pkgs.callPackage ./nix/wrapper.nix {
        inherit freesmlauncher-unwrapped;
      };

      freesmlauncher-unwrapped-debug = freesmlauncher-unwrapped.overrideAttrs {
        cmakeBuildType = "Debug";
        dontStrip = true;
      };

      freesmlauncher-debug = pkgs.callPackage ./nix/wrapper.nix {
        freesmlauncher-unwrapped = freesmlauncher-unwrapped-debug;
      };
    in {
      inherit freesmlauncher freesmlauncher-unwrapped freesmlauncher-debug freesmlauncher-unwrapped-debug;

      default = freesmlauncher;
    });

    devShells = forEachSystem (system: let
      pkgs = import nixpkgs {
        inherit system;
        overlays = [self.overlays.default];
      };
    in {
      default = pkgs.mkShell {
        inputsFrom = [pkgs.freesmlauncher-unwrapped];

        packages = with pkgs; [
          ccache
          ninja
        ];
      };
    });

    formatter = forEachSystem (
      system:
        (import nixpkgs {inherit system;}).alejandra
    );
  };
}
