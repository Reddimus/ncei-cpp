# Contributing

Clone the repository, install CMake 3.31+, a C++23 compiler, libcurl 7.85+
with thread-safe initialization, Python 3, and clang-format. Glaze and
GoogleTest are fetched by CMake. See README for compiler versions.

```sh
make build
make test
make lint
make consumers
```

Use `make debug` for a separate Debug build. For sanitizers, configure with
`-DNCEI_ENABLE_SANITIZERS=ON`; Debug alone does not enable them.
`make format` updates formatting. `make install-hooks` installs a lint-only
hook and preserves custom hooks.

Use C++23, tabs, 100 columns, and explicit local types. Iterators,
structured bindings, and lambda closures may use auto. Use the shared JSON
read options and add regression coverage for behavioral changes. Expected
request/parse errors use Result; date utilities validate and throw. Ordinary
allocation failures may throw.

Keep comments and docs concise. Describe the observable change and tests in
PRs. Update CHANGELOG.md for user-visible changes. All CI checks must pass
before merge. Security reports follow SECURITY.md.

For releases, merge the version and changelog update through a PR. Tag the
verified main commit with a signed vX.Y.Z tag and push it; the release
workflow publishes matching changelog notes. Use a minor version for API or
build-requirement changes and a patch version for compatible fixes.

Contributions are licensed under MIT.
