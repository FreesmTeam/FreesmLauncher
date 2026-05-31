{
  lib,
  self,
  libnbtplusplus,
  stdenv,
  cmake,
  ninja,
  jdk17,
  stripJavaArchivesHook,
  kdePackages,
  cmark,
  qrencode,
  zlib,
  tomlplusplus,
  ghc_filesystem,
  libarchive,
  darwin,
  gamemode,
  nix-filter,
  msaClientID ? null,
  gamemodeSupport ? stdenv.hostPlatform.isLinux,
}: let
  isDarwin = stdenv.hostPlatform.isDarwin;
in
  assert lib.assertMsg (gamemodeSupport -> stdenv.hostPlatform.isLinux)
  "gamemodeSupport only on linux";
    stdenv.mkDerivation {
      pname = "freesmlauncher-unwrapped";
      version = self.shortRev or self.dirtyShortRev or "_git";
      src = nix-filter.lib {
        root = self;
        include = [
          "buildconfig"
          "cmake"
          "launcher"
          "libraries"
          "program_info"
          "tests"
          ../docs/COPYING.md
          ../CMakeLists.txt
        ];

        # Some fetchers leave submodules directories empty instead of omitting them, causing Garnix CI cache misses.
        exclude = [
          "libraries/libnbtplusplus"
        ];
      };

      postUnpack = ''
        ln -s ${libnbtplusplus} source/libraries/libnbtplusplus
      '';

      nativeBuildInputs = [
        cmake
        ninja
        kdePackages.extra-cmake-modules
        jdk17
        stripJavaArchivesHook
      ];

      buildInputs =
        [
          cmark
          ghc_filesystem
          kdePackages.qtbase
          kdePackages.qtnetworkauth
          kdePackages.quazip
          libarchive
          tomlplusplus
          qrencode
          zlib
        ]
        ++ lib.optionals isDarwin [darwin.apple_sdk.frameworks.Cocoa]
        ++ lib.optionals gamemodeSupport [gamemode];

      cmakeFlags =
        [(lib.cmakeFeature "Launcher_BUILD_PLATFORM" "nixpkgs")]
        ++ lib.optionals (msaClientID != null) [
          (lib.cmakeFeature "Launcher_MSA_CLIENT_ID" msaClientID)
        ]
        ++ lib.optionals (lib.versionOlder kdePackages.qtbase.version "6")
        [
          (lib.cmakeFeature "Launcher_QT_VERSION_MAJOR" "5")
        ]
        ++ lib.optionals stdenv.hostPlatform.isDarwin [
          (lib.cmakeFeature "INSTALL_BUNDLE" "nodeps")
          (lib.cmakeFeature "MACOSX_SPARKLE_UPDATE_FEED_URL" "''")
          (lib.cmakeFeature "CMAKE_INSTALL_PREFIX" "${placeholder "out"}/Applications/")
        ];

      doCheck = true;
      dontWrapQtApps = true;
      enableParallelBuilding = true;

      meta = {
        description = "Prism Launcher fork aimed to provide a free way to play Minecraft";
        longDescription = ''
          Freesm Launcher is a custom launcher for Minecraft that allows you
          to easily manage multiple installations of Minecraft at once and login
          with offline account without any restrictions.
        '';
        homepage = "https://freesmlauncher.org/";
        license = lib.licenses.gpl3Only;
        platforms = lib.platforms.linux ++ lib.platforms.darwin;
        mainProgram = "freesmlauncher";
        maintainers = with lib.maintainers; [s0me1newithhand7s];
      };
    }
