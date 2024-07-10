{
  inputs,
  self,
  ...
}: {
  perSystem = {
    lib,
    pkgs,
    ...
  }: {
    packages = let
      ourPackages = lib.fix (final: self.overlays.default final pkgs);
    in {
      inherit
        (ourPackages)
        freesmlauncher-unwrapped
        freesmlauncher
        ;
      default = ourPackages.freesmlauncher;
    };
  };

  flake = {
    overlays.default = final: prev: let
      version = builtins.substring 0 8 self.lastModifiedDate or "dirty";
    in {
      freesmlauncher-unwrapped = prev.qt6Packages.callPackage ./pkg {
        inherit (inputs) libnbtplusplus;
        inherit ((final.darwin or prev.darwin).apple_sdk.frameworks) Cocoa;
        inherit version;
      };

      freesmlauncher = prev.qt6Packages.callPackage ./pkg/wrapper.nix {
        inherit (final) freesmlauncher-unwrapped;
      };
    };
  };
}
