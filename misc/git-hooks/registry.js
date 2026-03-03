/**
 * Registry of built-in checks and file sources.
 *
 * After esbuild bundles the linter, dynamic `import(path)` won't work
 * for built-in modules. Instead, linter-config.json references built-ins
 * by their export name (e.g. "CrlfCheck") and we resolve them here.
 *
 * Custom / user-provided checks can still use "module" + "export" in config.
 */

// --- checks ---
import { CrlfCheck } from "./checks/crlf-check.js";
import { LinelintCheck } from "./checks/linelint-check.js";
import { ClangFormatCheck } from "./checks/clang-format-check.js";
import { PairedFilesCheck } from "./checks/paired-files-check.js";

// --- file sources ---
import { AllFilesSource } from "./file-sources/all-files-source.js";
import { StagedFilesSource } from "./file-sources/staged-files-source.js";
import { DiffBaseSource } from "./file-sources/diff-base-source.js";

export const builtinRegistry = {
  // checks
  CrlfCheck,
  LinelintCheck,
  ClangFormatCheck,
  PairedFilesCheck,
  // file sources
  AllFilesSource,
  StagedFilesSource,
  DiffBaseSource,
};
