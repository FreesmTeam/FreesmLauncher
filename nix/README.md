> [!IMPORTANT]
> We are using **Garnix CI** for binary caching.
> To add the binary cache manually, proceed to read [this](https://garnix.io/docs/caching).
>
> We also use **[Cachix](https://app.cachix.org/cache/freesmlauncher#pull)** as a fallback for binary caching.
> For more information, check out [this guide](https://docs.cachix.org/getting-started#using-binaries-with-nix).

---

### <div align="center"> Using on NixOS / Nixpkgs </div>

Currently, **Freesm** isn't in `nixpkgs` (yet?). To use it, you'll need to add it in your `flake.nix`:

```nix
{
    inputs = {
        freesmlauncher = {
            url = "github:FreesmTeam/FreesmLauncher";
            inputs = {
                nixpkgs = {
                    follows = "nixpkgs";
                };
            };
        };
    };
    outputs = {
        self,
        nixpkgs,
        home-manager,
        freesmlauncher,
        ...
    } @ inputs :
    ... # rest of flake.
}
```

After that, you can add freesmlauncher to environment.systemPackages, users.users.<>.packages, or home.packages.
