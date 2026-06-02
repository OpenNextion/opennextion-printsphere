# Decisions

## 2026-06-01 - Demo-First Scope

Decision:

The first project target is a working demo, not a production release.

Consequences:

- 24 hour stability testing is not required for initial milestones.
- OTA polish and broad multi-printer regression are deferred.
- Short smoke tests are acceptable during bring-up.

## 2026-06-01 - Main Thread Owns Coordination

Decision:

The main thread owns task dispatch, subthread summaries, Git integration, and acceptance.

Consequences:

- Subthreads can analyze or implement scoped tasks.
- Subthreads do not decide global toolchain or project structure independently.
- Main thread must explicitly read completed subthread outputs.

## 2026-06-01 - Board Profile Strategy

Decision:

Use an ONX3248G035 board profile and keep hardware differences inside the board support layer where practical.

Consequences:

- Avoid scattering ONX checks across application code.
- Preserve PrintSphere Bambu protocol and configuration logic where possible.
- Expose PrintSphere-compatible BSP functions for display, touch, brightness, I2C, and power where practical.

## 2026-06-01 - UI Must Be Reworked for 320 x 480

Decision:

Do not simply scale the original 466 x 466 circular display layout.

Consequences:

- Build a 320 x 480 demo layout.
- Simplify AMS and camera pages for the first demo.
- Favor readable status display over feature parity.

## 2026-06-01 - Git Initialization Blocked

Decision:

Git is the intended project control mechanism. Initialization was temporarily blocked because the Codex runtime could not create `.git` in this workspace.

Evidence:

```text
/Users/alex/Documents/codex_project/Nextion_project_PrintSphere/.git: Operation not permitted
```

Consequences:

- The block was resolved on 2026-06-02 by approving `git init` outside the Codex sandbox.
- `.tools/` and `.DS_Store` are ignored to keep local toolchains and system files out of source control.
- Baseline commit was created on 2026-06-02 before feature work.
