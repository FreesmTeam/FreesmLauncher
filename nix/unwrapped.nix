{
  lib,
  self,
  zlib,
  stdenv,
  darwin,
  kdePackages,
  tomlplusplus,
  ghc_filesystem,
  stripJavaArchivesHook,

  cmake,
  ninja,
  jdk17,
  cmark,
  gamemode,
  nix-filter,
  libnbtplusplus,
  extra-cmake-modules,

  msaClientID ? null,
  gamemodeSupport ? stdenv.hostPlatform.isLinux,
}:
assert lib.assertMsg (
  gamemodeSupport -> stdenv.hostPlatform.isLinux
) "gamemodeSupport is only available on Linux.";

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
      ../COPYING.md
      ../CMakeLists.txt
    ];
  };

  postUnpack = ''
    rm -rf source/libraries/libnbtplusplus
    ln -s ${libnbtplusplus} source/libraries/libnbtplusplus
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
      tomlplusplus
      zlib
    ]
    ++ lib.optionals stdenv.hostPlatform.isDarwin [
      darwin.apple_sdk.frameworks.Cocoa
    ]
    ++ lib.optional gamemodeSupport gamemode;

  hardeningEnable = lib.optionals stdenv.hostPlatform.isLinux [
    "pie"
  ];

  cmakeFlags =
    [
      (lib.cmakeFeature "Launcher_BUILD_PLATFORM" "nixpkgs")
    ]
    ++ lib.optionals (msaClientID != null) [
      (lib.cmakeFeature "Launcher_MSA_CLIENT_ID" (toString msaClientID))
    ]
    ++ lib.optionals (lib.versionOlder kdePackages.qtbase.version "6") [
      (lib.cmakeFeature "Launcher_QT_VERSION_MAJOR" "5")
    ]
    ++ lib.optionals stdenv.hostPlatform.isDarwin [
      (lib.cmakeFeature "INSTALL_BUNDLE" "nodeps")
      (lib.cmakeFeature "MACOSX_SPARKLE_UPDATE_FEED_URL" "''")
      (lib.cmakeFeature "CMAKE_INSTALL_PREFIX" "${placeholder "out"}/Applications/")
    ];

  doCheck = true;

  dontWrapQtApps = true;

  meta = {
    description = "Prism Launcher fork aimed to provide a free way to play Minecraft";
    homepage = "https://freesmlauncher.windstone.space/";
    platforms = lib.platforms.linux ++ lib.platforms.darwin;
    mainProgram = "freesmlauncher";
    license = lib.licenses.gpl3Only;
    longDescription = ''
      Freesm Launcher is a custom launcher for Minecraft that allows you 
      to easily manage multiple installations of Minecraft at once and login 
      with offline account without any restrictions.
    '';
    maintainers = with lib.maintainers; [
      s0me1newithhand7s
      windstone
      kaeeraa
    ];
  };
}
