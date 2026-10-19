# Maintainer Rules

Maintainers are responsible for enforcing the rules in this document. So, if you just want to contribute you may have no idea about what's going on in this document.

## Messaging Protocol

If PR changes messaging protocol, make sure that the author has updated `kMessagingProtocolVersion` in `skymp5-server/cpp/mp_common/Config.h`
and breaking change is marked in PR's name.

## Tree

Merge commits are avoided in the source tree like in [microsoft/vcpkg](https://github.com/microsoft/vcpkg).

## Commit Names

Commit names follow [Conventional Commits Specification](https://www.conventionalcommits.org/en/v1.0.0/).

Commit types used (sorted by priority):

- release - for releasing new versions of SP and other projects.
- feat - API or functionality change, both backward-compatible or not.
- fix - bugfix or crash fix.
- tests - changes in tests.
- perf - changes in code that target improving performance.
- docs - changes in text files.
- refact - changes in code that target improving readability or rewriting code in different lang/style.
- internal - fixes or changes in the build system, catalog structure, or anything that doesn't execute on the user's machine.

Repo subdirectories are used to name commit scopes. Commits without scope are allowed.

In practice, you should use `skymp5-server` as the scope of your commit if all changes are in the `skymp5-server` folder, etc.

`docs` and `tests` commit types must not have scope.

All commit types except `fix` requires to start with verbs (add, make, etc).

Examples:

```
feat(skymp5-server): add feature to choose default spawn points
internal: make server's node addon buildable via top-level CMakeLists
fix: server startup
release(skyrim-platform): version 2.1
```

Note that `BREAKING CHANGE` footer isn't used. Only `!` sign.

## GitHub Workflow Diagnostics

Workflow files in `.github/workflows` are often checked by an editor-side YAML/Actions validator.
This validator can report warnings that look similar but have different severity.

### Actionable warnings (must fix)

These are usually real logic issues in workflow context wiring:

- Missing step id for a referenced output (for example `steps.buildx.outputs.name` while the step has no `id: buildx`)
- Referencing `${{ env.VAR }}` in expression contexts even though the value is produced dynamically in a previous step and should be exposed via `$GITHUB_OUTPUT`
- Referencing `${{ env.* }}` in a step where the value was never exported

Recommended pattern:

1. Produce dynamic values in a step with `id:` and write to `$GITHUB_OUTPUT`
2. Consume values in expressions via `${{ steps.<id>.outputs.<name> }}`
3. In shell scripts, prefer shell variables (`$VAR`) after exporting to `$GITHUB_ENV`

### Non-actionable warnings (usually safe)

Warnings like `Context access might be invalid: SOME_SECRET_NAME` for `secrets.*`
often mean only that the local validator cannot verify repository or organization secret names.

If the secret names are intentional and configured in GitHub, these warnings can be ignored.

### Maintainer checklist for workflow warning triage

1. Fix output/id/env wiring issues first.
2. Re-run diagnostics on `.github/workflows`.
3. If remaining warnings are only unknown `secrets.*` names, treat as informational.
4. If uncertain, verify secret names in repository/org settings before changing workflow code.
