# Repository

Maintainers are responsible for enforcing the rules in this document. So, if you just want to contribute you may have no idea about what's going on in this document.

## Tree

Merge commits are avoided in the source tree like in [microsoft/vcpkg](https://github.com/microsoft/vcpkg).

## Commit Names

Commit names follow [Conventional Commits Specification](https://www.conventionalcommits.org/en/v1.0.0/).

Commit types used (sorted by priority):
* feat - API or functionality change, both backward-compatible or not.
* fix - bugfix or crash fix.
* tests - changes in tests.
* perf - changes in code that target improving performance.
* docs - changes in text files.
* internal - fixes or changes in the build system, catalog structure, or anything that doesn't execute on the user's machine.

Note that `BREAKING CHANGE` footer isn't used. Only `!` sign.