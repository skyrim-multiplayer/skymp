import { Project, SyntaxKind } from "ts-morph";
import type { MethodDeclaration } from "ts-morph";
import type {
  RegisteredPatch,
  PatchResult,
  PrefixPatchInfo,
  PostfixPatchInfo,
  TranspilerPatchInfo,
} from "./types";

/**
 * Applies registered patches to a ts-morph Project over the copied source tree.
 *
 * For each patch:
 *   1. Locate the target file, class, and method in the project.
 *   2. Run any @Transpiler patches first (direct AST access).
 *   3. Inject @Prefix and @Postfix IIFEs around the original body.
 *   4. Save the modified source file back to disk.
 */
export class AstEngine {
  constructor(private readonly project: Project) {}

  applyPatch(patch: RegisteredPatch): PatchResult {
    const { target, patches } = patch;

    try {
      // Normalize to forward slashes for cross-platform path matching.
      const normalizedTarget = target.file.replace(/\\/g, "/");

      const sourceFile = this.project.getSourceFile(
        (sf) => sf.getFilePath().replace(/\\/g, "/").endsWith("/" + normalizedTarget)
          || sf.getFilePath().replace(/\\/g, "/").endsWith(normalizedTarget)
      );

      if (!sourceFile) {
        return {
          target,
          status: "skipped",
          error: new Error(`Source file not found in project: ${target.file}`),
        };
      }

      const classDecl = sourceFile.getClass(target.class);
      if (!classDecl) {
        return {
          target,
          status: "skipped",
          error: new Error(`Class "${target.class}" not found in ${target.file}`),
        };
      }

      const methodDecl = classDecl.getMethod(target.method);
      if (!methodDecl) {
        return {
          target,
          status: "skipped",
          error: new Error(
            `Method "${target.method}" not found in class "${target.class}" in ${target.file}`
          ),
        };
      }

      const prefixes = patches.filter((p): p is PrefixPatchInfo => p.kind === "prefix");
      const postfixes = patches.filter((p): p is PostfixPatchInfo => p.kind === "postfix");
      const transpilers = patches.filter((p): p is TranspilerPatchInfo => p.kind === "transpiler");

      // Transpilers run first — they may restructure the method body entirely.
      for (const tp of transpilers) {
        tp.fn(methodDecl);
      }

      // Prefix + postfix injection happens after any transpiler edits.
      if (prefixes.length > 0 || postfixes.length > 0) {
        injectPrefixPostfix(methodDecl, prefixes, postfixes);
      }

      sourceFile.saveSync();

      return { target, status: "applied" };
    } catch (err) {
      return {
        target,
        status: "failed",
        error: err instanceof Error ? err : new Error(String(err)),
      };
    }
  }
}

/**
 * Injects prefix and postfix IIFEs around the original method body using
 * methodDecl.setBodyText(), which re-parses the new text in place.
 *
 * The generated pattern for a void method with one prefix and one postfix:
 *
 *   const __instance = this;
 *   const __prefixResult = (function(__instance: T, p1: P1): boolean {
 *     <prefix body>
 *   })(__instance, p1);
 *   if (__prefixResult === false) { return; }
 *
 *   <original body statements>
 *
 *   const __result = undefined;
 *   (function(__instance: T, p1: P1, __result: void): void {
 *     <postfix body>
 *   })(__instance, p1, __result);
 *
 * For non-void return methods, the original body is wrapped in a .call(this)
 * IIFE to capture the return value into __result.
 */
function injectPrefixPostfix(
  methodDecl: MethodDeclaration,
  prefixes: PrefixPatchInfo[],
  postfixes: PostfixPatchInfo[]
): void {
  const body = methodDecl.getBodyOrThrow();
  const block = body.asKindOrThrow(SyntaxKind.Block);

  // Capture original statements before any modification.
  const originalStatements = block.getStatements();
  const originalBodyText =
    originalStatements.length > 0
      ? originalStatements.map((s) => s.getText()).join("\n")
      : "";

  // Determine return type for __result handling.
  const returnTypeNode = methodDecl.getReturnTypeNode();
  const returnTypeText = returnTypeNode ? returnTypeNode.getText().trim() : "void";
  const isVoidReturn =
    returnTypeText === "void" ||
    returnTypeText === "Promise<void>" ||
    returnTypeText === "never";

  // Original method parameter names (used as call args in IIFEs).
  const origParamNames = methodDecl.getParameters().map((p) => p.getName());

  const lines: string[] = [];

  // __instance capture — provides 'this' to IIFEs (which have their own 'this').
  lines.push("const __instance = this;");

  // Prefix IIFEs.
  for (let i = 0; i < prefixes.length; i++) {
    const prefix = prefixes[i];
    const resultVar = i === 0 ? "__prefixResult" : `__prefixResult_${i}`;
    const iifeSig = buildParamList(prefix.paramNames, prefix.paramTypes);
    const callArgs = buildCallArgs(prefix.paramNames, origParamNames);

    lines.push(
      `const ${resultVar} = (function(${iifeSig}): ${prefix.returnType} {`,
      indent(prefix.bodyText),
      `})(${callArgs});`,
      `if (${resultVar} === false) { return; }`
    );
  }

  // Original body — optionally wrapped to capture return value.
  if (postfixes.length > 0 && !isVoidReturn) {
    // Wrap original body in a sub-IIFE so early returns are captured.
    lines.push(
      `const __result: ${returnTypeText} = (function(this: typeof __instance): ${returnTypeText} {`,
      indent(originalBodyText),
      `}).call(this);`
    );
  } else if (postfixes.length > 0 && isVoidReturn) {
    // Inline the original body, then declare __result as undefined.
    if (originalBodyText) {
      lines.push(originalBodyText);
    }
    lines.push("const __result = undefined;");
  } else {
    // No postfix — just inline the original body.
    if (originalBodyText) {
      lines.push(originalBodyText);
    }
  }

  // Postfix IIFEs.
  for (const postfix of postfixes) {
    const iifeSig = buildParamList(postfix.paramNames, postfix.paramTypes);
    const callArgs = buildCallArgs(postfix.paramNames, origParamNames);

    lines.push(
      `(function(${iifeSig}): void {`,
      indent(postfix.bodyText),
      `})(${callArgs});`
    );
  }

  methodDecl.setBodyText(lines.join("\n"));
}

/**
 * Builds an IIFE parameter signature string.
 * Example: (["__instance","player"], ["Login","Player"]) → "__instance: Login, player: Player"
 */
function buildParamList(names: string[], types: string[]): string {
  return names.map((name, i) => `${name}: ${types[i] ?? "any"}`).join(", ");
}

/**
 * Maps IIFE parameter names to actual call arguments.
 *
 * Convention:
 *   - "__instance" always maps to "__instance"
 *   - "__result" always maps to "__result"
 *   - All other params are matched positionally to the original method params
 *
 * Example:
 *   prefixParams: ["__instance", "player"]
 *   origParams:   ["player"]
 *   → "__instance, player"
 */
function buildCallArgs(prefixParamNames: string[], origParamNames: string[]): string {
  const args: string[] = [];
  let origIdx = 0;

  for (const name of prefixParamNames) {
    if (name === "__instance") {
      args.push("__instance");
    } else if (name === "__result") {
      args.push("__result");
    } else {
      args.push(origParamNames[origIdx] ?? name);
      origIdx++;
    }
  }

  return args.join(", ");
}

/** Indents each line of a block by two spaces for IIFE body readability. */
function indent(text: string): string {
  if (!text.trim()) return "";
  return text
    .split("\n")
    .map((line) => `  ${line}`)
    .join("\n");
}
