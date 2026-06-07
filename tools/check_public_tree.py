#!/usr/bin/env python3
"""Check that the tracked public repository tree stays release-safe."""

from __future__ import annotations

import subprocess
import sys
from pathlib import PurePosixPath


ALLOWED_DOC_FILES = {
    "docs/BUILD_AND_FLASH.md",
    "docs/PUBLICATION_POLICY.md",
    "docs/RELEASE_FLASHING.md",
    "docs/SUPPORTED_BOARDS.md",
}

ALLOWED_DOC_IMAGE_EXTENSIONS = {".jpg", ".jpeg", ".png", ".webp", ".gif"}

FORBIDDEN_EXACT = {
    "docs/A1_MINI_LAN_BRINGUP.md",
    "docs/BUILD_FLASH.md",
    "docs/DECISIONS.md",
    "docs/DEV_ENV.md",
    "docs/M2_BSP_ACCEPTANCE.md",
    "docs/M3_BUILD_PROFILE_PLAN.md",
    "docs/M3_ONX_BSP_INTEGRATION.md",
    "docs/M4_ONX_BOARD_DIAGNOSTICS.md",
    "docs/MILESTONES.md",
    "docs/ONX3248G035_HARDWARE_CONFIG.md",
    "docs/ONX_BSP_NOTES.md",
    "docs/PORTING_ARCHITECTURE_BOUNDARIES.md",
    "docs/PROJECT_CONTEXT.md",
    "docs/THREADS.md",
    "docs/UI_DESIGN_SPEC.md",
    "docs/UI_DESIGN_SPEC_LANDSCAPE.md",
    "docs/ui_design_preview.html",
    "docs/ui_design_landscape_preview.html",
    "release/beta/README.md",
}

FORBIDDEN_PREFIXES = (
    "docs/assets/",
    "examples/",
    "release/beta/",
)

FORBIDDEN_SUFFIXES = (
    ".DS_Store",
    ".bin",
    ".elf",
    ".map",
)


def git_ls_files() -> list[str]:
    output = subprocess.check_output(["git", "ls-files"], text=True)
    return [line.strip() for line in output.splitlines() if line.strip()]


def add_violation(violations: list[str], path: str, reason: str) -> None:
    violations.append(f"{path}: {reason}")


def check_path(path: str, violations: list[str]) -> None:
    posix = PurePosixPath(path)

    if path in FORBIDDEN_EXACT:
        add_violation(violations, path, "internal publication file is forbidden")

    for prefix in FORBIDDEN_PREFIXES:
        if path.startswith(prefix):
            add_violation(violations, path, f"forbidden prefix {prefix}")

    for suffix in FORBIDDEN_SUFFIXES:
        if path.endswith(suffix):
            add_violation(violations, path, f"forbidden generated/private suffix {suffix}")

    if path.startswith("release/"):
        add_violation(violations, path, "release files must be GitHub Release assets, not git files")

    if path.startswith("docs/"):
        if path in ALLOWED_DOC_FILES:
            return
        if path.startswith("docs/images/"):
            if posix.suffix.lower() in ALLOWED_DOC_IMAGE_EXTENSIONS:
                return
            add_violation(violations, path, "docs/images only allows web image files")
            return
        add_violation(violations, path, "docs file is not in the public allowlist")


def main() -> int:
    violations: list[str] = []
    for path in git_ls_files():
        check_path(path, violations)

    if violations:
        print("Public tree check failed. Remove or explicitly review these tracked files:")
        for violation in violations:
            print(f"  - {violation}")
        return 1

    print("Public tree check passed.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
