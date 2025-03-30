{
  description = "A custom launcher for Minecraft that allows you to easily manage multiple installations of Minecraft at once (Fork of MultiMC)";

  nixConfig = {
    extra-substituters = [ "https://prismlauncher.cachix.org" ];
    extra-trusted-public-keys = [
      "prismlauncher.cachix.org-1:9/n/FGyABA2jLUVfY+DEp4hKds/rwO+SCOtbOkDzd+c="
    ];
  };

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";

    libnbtplusplus = {
      url = "github:PrismLauncher/libnbtplusplus";
      flake = false;
    };
  };

  outputs =
    {
      self,
      nixpkgs,
      libnbtplusplus,
    }:

    let
      inherit (nixpkgs) lib;

      # While we only officially support aarch and x86_64 on Linux and MacOS,
      # we expose a reasonable amount of other systems for users who want to
      # build for most exotic platforms
      systems = lib.systems.flakeExposed;

      forAllSystems = lib.genAttrs systems;
      nixpkgsFor = forAllSystems (system: nixpkgs.legacyPackages.${system});
    in

    {
      checks = forAllSystems (
        system:

        let
          pkgs = nixpkgsFor.${system};
          llvm = pkgs.llvmPackages_19;
        in

        {
          formatting =
            pkgs.runCommand "check-formatting"
              {
                nativeBuildInputs = with pkgs; [
                  deadnix
                  llvm.clang-tools
                  markdownlint-cli
                  nixfmt-rfc-style
                  statix
                ];
              }
              ''
                cd ${self}

                echo "Running clang-format...."
                clang-format --dry-run --style='file' --Werror */**.{c,cc,cpp,h,hh,hpp}

                echo "Running deadnix..."
                deadnix --fail

                echo "Running markdownlint..."
                markdownlint --dot .

                echo "Running nixfmt..."
                find -type f -name '*.nix' -exec nixfmt --check {} +

                echo "Running statix"
                statix check .

                touch $out
              '';
        }
      );

      devShells = forAllSystems (
        system:

        let
          pkgs = nixpkgsFor.${system};
          llvm = pkgs.llvmPackages_19;

          packages' = self.packages.${system};

          # Re-use our package wrapper to wrap our development environment
          qt-wrapper-env = packages'.prismlauncher.overrideAttrs (old: {
            name = "qt-wrapper-env";

            # Required to use script-based makeWrapper below
            strictDeps = true;

            # We don't need/want the unwrapped Prism package
            paths = [ ];

            nativeBuildInputs = old.nativeBuildInputs or [ ] ++ [
              # Ensure the wrapper is script based so it can be sourced
              pkgs.makeWrapper
            ];

            # Inspired by https://discourse.nixos.org/t/python-qt-woes/11808/10
            buildCommand = ''
              makeQtWrapper ${lib.getExe pkgs.runtimeShellPackage} "$out"
              sed -i '/^exec/d' "$out"
            '';
          });
        in

        {
          default = pkgs.mkShell {
            inputsFrom = [ packages'.prismlauncher-unwrapped ];

            packages = with pkgs; [
              ccache
              llvm.clang-tools
            ];

            cmakeFlags = packages'.prismlauncher-unwrapped.cmakeFlags ++ [
              "-GNinja"
              "-Bbuild"
            ];

            shellHook = ''
              echo "Sourcing ${qt-wrapper-env}"
              source ${qt-wrapper-env}

              if [ ! -f compile_commands.json ]; then
                cmake $cmakeFlags -D CMAKE_BUILD_TYPE=Debug
                ln -s {build/,}compile_commands.json
              fi
            '';
          };
        }
      );

      formatter = forAllSystems (system: nixpkgsFor.${system}.nixfmt-rfc-style);

      overlays.default = final: prev: {
        prismlauncher-unwrapped = prev.callPackage ./nix/unwrapped.nix {
          inherit
            libnbtplusplus
            self
            ;
        };

        prismlauncher = final.callPackage ./nix/wrapper.nix { };
      };

      packages = forAllSystems (
        system:

        let
          pkgs = nixpkgsFor.${system};

          # Build a scope from our overlay
          prismPackages = lib.makeScope pkgs.newScope (final: self.overlays.default final pkgs);

          # Grab our packages from it and set the default
          packages = {
            inherit (prismPackages) prismlauncher-unwrapped prismlauncher;
            default = prismPackages.prismlauncher;
          };
        in

        # Only output them if they're available on the current system
        lib.filterAttrs (_: lib.meta.availableOn pkgs.stdenv.hostPlatform) packages
      );

      # We put these under legacyPackages as they are meant for CI, not end user consumption
      legacyPackages = forAllSystems (
        system:

        let
          packages' = self.packages.${system};
          legacyPackages' = self.legacyPackages.${system};
        in

        {
          prismlauncher-debug = packages'.prismlauncher.override {
            prismlauncher-unwrapped = legacyPackages'.prismlauncher-unwrapped-debug;
          };

          prismlauncher-unwrapped-debug = packages'.prismlauncher-unwrapped.overrideAttrs {
            cmakeBuildType = "Debug";
            dontStrip = true;
          };
        }
      );
    };
}
