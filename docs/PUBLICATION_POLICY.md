# Publication Policy

This repository is the public product repository for OpenNextion-printsphere.
It must not publish local development coordination records, bring-up notes,
thread handoff documents, smoke-test projects, generated firmware files, or
private environment notes.

## Public Documentation Scope

The public `docs/` tree is intentionally small:

- `docs/BUILD_AND_FLASH.md`
- `docs/RELEASE_FLASHING.md`
- `docs/SUPPORTED_BOARDS.md`
- `docs/PUBLICATION_POLICY.md`
- `docs/images/`

Any new public document must be reviewed by the main architecture thread before
it is committed. If the document is approved, `tools/check_public_tree.py` must
be updated in the same commit so the allowlist remains explicit.

## Files That Must Stay Out Of The Public Tree

The public repository must not track:

- Codex thread coordination records
- milestone or planning records
- local environment notes
- board bring-up acceptance logs
- UI design preview HTML files
- internal flash/debug procedure records
- smoke-test `examples/` projects
- release candidate or beta firmware staging folders
- generated firmware binaries

Firmware files are GitHub Release assets only. They are not committed to git.

## Why This Guard Exists

The initial public `main` was created from the working development tree. That
published internal documents such as milestone notes, thread coordination
records, bring-up logs, and smoke-test examples. The README screenshot update
did not add those files; it only exposed that the initial public tree had not
been mechanically checked.

The root cause was that publication scope existed as discussion and thread
instructions, but not as an executable repository check.

## Required Release Flow

Before any public repository push:

1. Confirm the staged file list is limited to the intended publication change.
2. Run:

   ```sh
   python3 tools/check_public_tree.py
   ```

3. Do not push if the check fails.
4. Do not move, rebuild, delete, or retag an existing release tag for README or
   documentation-only updates.

`v0.1.0` remains the frozen firmware release tag. Later README or documentation
cleanup commits may move `main`, but they must not move `v0.1.0` or change the
existing GitHub Release assets.
