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
  gamemode,
  extra-cmake-modules,
  msaClientID ? null,
  gamemodeSupport ? stdenv.hostPlatform.isLinux,
}:
assert lib.assertMsg (gamemodeSupport -> stdenv.hostPlatform.isLinux)
"gamemodeSupport only on linux";
  stdenv.mkDerivation {
    pname = "freesmlauncher-unwrapped";
    version = self.shortRev or self.dirtyShortRev or "_git";
    src = with lib.fileset;
      toSource {
        root = ../.;
        fileset = unions [
          ../buildconfig
          ../cmake
          ../launcher
          ../libraries
          ../program_info
          ../tests

          ../CMakeLists.txt
          ../docs/COPYING.md
        ];
      };

    postUnpack = let
      folder = "source/libraries/libnbtplusplus";
    in ''
      rm -rf ${folder}
      ln -s ${libnbtplusplus} ${folder}
    '';

    nativeBuildInputs = [
      cmake
      ninja
      extra-cmake-modules
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
      ++ lib.optionals gamemodeSupport [gamemode];

    cmakeFlags =
      [(lib.cmakeFeature "Launcher_BUILD_PLATFORM" "nixpkgs")]
      ++ lib.optionals (msaClientID != null) [
        (lib.cmakeFeature "Launcher_MSA_CLIENT_ID" msaClientID)
      ]
      ++ lib.optionals stdenv.hostPlatform.isDarwin [
        (lib.cmakeFeature "INSTALL_BUNDLE" "nodeps")
        (lib.cmakeFeature "MACOSX_SPARKLE_UPDATE_FEED_URL" "''")
        (lib.cmakeFeature "CMAKE_INSTALL_PREFIX" "${placeholder "out"}/Applications/")
      ];

    doCheck = stdenv.hostPlatform.isLinux;
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
