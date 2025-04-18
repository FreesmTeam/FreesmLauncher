{
  runCommand,
  deadnix,
  llvmPackages_18,
  markdownlint-cli,
  alejandra,
  statix,
  self,
}: {
  formatting =
    runCommand "check-formatting"
    {
      nativeBuildInputs = [
        deadnix
        llvmPackages_18.clang-tools
        markdownlint-cli
        alejandra
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

      echo "Running alejandra..."
      alejandra --check .

      echo "Running statix"
      statix check .

      touch $out
    '';
}
