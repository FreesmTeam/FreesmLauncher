> [!NOTE]
> We are using Garnix CI for binary caching. 
> Proceed to read (this)[https://garnix.io/docs/caching],
> if you want to add binary cache manually.

<h3 align="center"> Using on nixos / nixpkgs </h3>
Freesm isn't in `nixpkgs` (yet?), so you need to add Freesm in `flake.nix`:

```Nix
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

And now you can add `freesmlauncher` in `environment.systemPackages`, `users.users.<>.packages` or `home.packages`.
