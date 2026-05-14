# NixOS

> [!IMPORTANT]
> We use **Garnix CI** for binary caching.
> To configure the cache manually, follow the official guide:
>
> - [Garnix binary cache guide](https://garnix.io/docs/ci/caching)
>
> We also provide a secondary cache through [**Cachix**](https://app.cachix.org/cache/freesmlauncher#pull).
> Additional information is available in the official
> [Cachix getting started guide](https://docs.cachix.org/getting-started#using-binaries-with-nix).

<div align="center">

# Running and installing on NixOS

This guide explains how to run and install **FreesmLauncher** on NixOS.

</div>

## Running without installation

```fish
nix run github:FreesmTeam/FreesmLauncher#freesmlauncher
```

## Installation

Add the flake input to your `flake.nix`:

```nix
{
  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";

    freesmlauncher = {
      url = "github:FreesmTeam/FreesmLauncher";
      inputs.nixpkgs.follows = "nixpkgs";
    };
  };

  outputs = { self, nixpkgs, freesmlauncher, ... }: {
    # your outputs
  };
}
```

### NixOS configuration

```nix
{ pkgs, system, freesmlauncher, ... }:

{
  environment.systemPackages = [
    freesmlauncher.packages.${system}.freesmlauncher
  ];
}
```

### Home Manager configuration

```nix
{ pkgs, system, freesmlauncher, ... }:

{
  home.packages = [
    freesmlauncher.packages.${system}.freesmlauncher
  ];
}
```

## Updating

To update the flake input:

```fish
nix flake update freesmlauncher
```

Or update all inputs:

```fish
nix flake update
```
