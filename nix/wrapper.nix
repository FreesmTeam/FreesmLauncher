{
  lib,
  symlinkJoin,
  kdePackages,
  addDriverRunpath,
  freesmlauncher-unwrapped,
  stdenv,
  alsa-lib,
  flite,
  gamemode,
  glfw3-minecraft,
  libGL,
  libX11,
  libXcursor,
  libXext,
  libXrandr,
  libXxf86vm,
  libjack2,
  libpulseaudio,
  libusb1,
  mesa-demos,
  openal,
  pciutils,
  pipewire,
  udev,
  vulkan-loader,
  xrandr,
  msaClientID ? null,
  controllerSupport ? stdenv.hostPlatform.isLinux,
  gamemodeSupport ? stdenv.hostPlatform.isLinux,
  textToSpeechSupport ? stdenv.hostPlatform.isLinux,
  jvmPack,
  jdks ? jvmPack.openjdk,
}:
assert lib.assertMsg (
  if (
    jdks != jvmPack.openjdk || jdks != jvmPack.temurin || jdks != jvmPack.allPack
    ) then builtins.trace "used jvmPack does not provide jdk8!" true
    else true
) "";
assert lib.assertMsg (
  controllerSupport -> stdenv.hostPlatform.isLinux
) "controllerSupport only has an effect on Linux.";
assert lib.assertMsg (
  textToSpeechSupport -> stdenv.hostPlatform.isLinux
) "textToSpeechSupport only has an effect on Linux."; let
  isLinux = stdenv.hostPlatform.isLinux;

  launcher = freesmlauncher-unwrapped.override {
    inherit msaClientID gamemodeSupport;
  };

  runtimePrograms = [mesa-demos pciutils xrandr];
  runtimeLibs =
    [
      stdenv.cc.cc.lib

      glfw3-minecraft
      openal

      alsa-lib
      libjack2
      libpulseaudio
      pipewire

      libGL
      libX11
      libXcursor
      libXext
      libXrandr
      libXxf86vm

      udev
      vulkan-loader
    ]
    ++ lib.optionals textToSpeechSupport [flite]
    ++ lib.optionals gamemodeSupport [gamemode.lib]
    ++ lib.optionals controllerSupport [libusb1];
in
  symlinkJoin {
    pname = "freesmlauncher";
    inherit (launcher) version meta;
    paths = [launcher];
    nativeBuildInputs = [kdePackages.wrapQtAppsHook];
    buildInputs = with kdePackages;
      [qtbase qtsvg]
      ++ lib.optional (lib.versionAtLeast qtbase.version "6" && isLinux) qtwayland;

    postBuild = ''
      wrapQtAppsHook
    '';

    qtWrapperArgs =
      ["--prefix FREESMLAUNCHER_JAVA_PATHS : ${lib.makeSearchPath "bin/java" jdks}"]
      ++ lib.optionals isLinux [
        "--prefix PATH : ${lib.makeBinPath runtimePrograms}"
        "--prefix LD_LIBRARY_PATH : ${addDriverRunpath.driverLink}/lib:${lib.makeLibraryPath runtimeLibs}"
      ];
  }
