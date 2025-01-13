# Contributions Guidelines

## Code formatting

Try to follow the existing formatting.
If there is no existing formatting, you may use `clang-format` with our included `.clang-format` configuration.

In general, in order of importance:

- Make sure your IDE is not messing up line endings or whitespace and avoid using linters.
- Prefer readability over dogma.
- Keep to the existing formatting.
- Indent with 4 space unless it's in a submodule.
- Keep lists (of arguments, parameters, initializers...) as lists, not paragraphs. It should either read from top to bottom, or left to right. Not both.

## Commit Messages

### Format

`{type}({scope})!: {subject}`

### Elements

- **Type**: Choose from the following list. If none of the types match, use `chore`.
  - `feat`: A new feature
  - `fix`: A bug fix
  - `docs`: Documentation only changes
  - `style`: Changes that do not affect the meaning of the code
  - `refactor`: Improving code structure
  - `perf`: A code change that improves performance
  - `test`: Adding missing tests or correcting existing tests
  - `build`: Changes that affect the build system or external dependencies
  - `chore`: Other changes that don't modify src or test files
  - `revert`: Reverts a previous commit
  - `release`: Releasing a new version
  - `ci`: Changes to our CI configuration
- **Scope**: Indicate the location of the changed files, e.g., `ui/widgets`. Do not include the root, like `launcher/ui/widgets`. Use the shared parent folder name for multiple files. Omit if changes are in the root.
- **Breaking Change**: If you're introducing a breaking change, append `!` to the type or scope, e.g., `feat(ui)!: Breaking change`.

- **Subject**: Brief description of the change.

### Guidelines

- Use imperative mood, e.g., "Add feature" instead of "Adding feature" or "Added feature".
- Limit to 50 characters, but can extend to 72 if necessary.
- Avoid ending with a period.

## Signing your work

In an effort to ensure that the code you contribute is actually compatible with the licenses in this codebase, we require you to sign-off all your contributions.

This can be done by appending `-s` to your `git commit` call, or by manually appending the following text to your commit message:

```text
<commit message>

Signed-off-by: Author name <Author email>
```

By signing off your work, you agree to the terms below:

```text
Developer's Certificate of Origin 1.1

By making a contribution to this project, I certify that:

(a) The contribution was created in whole or in part by me and I
    have the right to submit it under the open source license
    indicated in the file; or

(b) The contribution is based upon previous work that, to the best
    of my knowledge, is covered under an appropriate open source
    license and I have the right under that license to submit that
    work with modifications, whether created in whole or in part
    by me, under the same open source license (unless I am
    permitted to submit under a different license), as indicated
    in the file; or

(c) The contribution was provided directly to me by some other
    person who certified (a), (b) or (c) and I have not modified
    it.

(d) I understand and agree that this project and the contribution
    are public and that a record of the contribution (including all
    personal information I submit with it, including my sign-off) is
    maintained indefinitely and may be redistributed consistent with
    this project or the open source license(s) involved.
```

These terms will be enforced once you create a pull request, and you will be informed automatically if any of your commits aren't signed-off by you.

As a bonus, you can also [cryptographically sign your commits][gh-signing-commits] and enable [vigilant mode][gh-vigilant-mode] on GitHub.

[gh-signing-commits]: https://docs.github.com/en/authentication/managing-commit-signature-verification/signing-commits
[gh-vigilant-mode]: https://docs.github.com/en/authentication/managing-commit-signature-verification/displaying-verification-statuses-for-all-of-your-commits

## Backporting to Release Branches

We use [automated backports](https://github.com/PrismLauncher/PrismLauncher/blob/develop/.github/workflows/backport.yml) to merge specific contributions from develop into `release` branches.

This is done when pull requests are merged and have labels such as `backport release-7.x` - which should be added along with the milestone for the release.
