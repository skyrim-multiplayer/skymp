import { Project, SourceFile, SyntaxKind } from "ts-morph";
import type { MethodDeclaration } from "ts-morph";
import type {
  SkyPatchTarget,
  RegisteredPatch,
  PatchInfo,
  PrefixPatchInfo,
  PostfixPatchInfo,
} from "./types";

/**
 * Internal shape stored when a @SkyPatch class registers itself.
 * Raw — not yet resolved into PatchInfo objects; resolution requires
 * reading the patch file AST via ts-morph (done later in resolveAll).
 */
export interface RawPatchEntry {
  target: SkyPatchTarget;
  /** The patch class constructor — used to find the class in AST by name */
  constructor: Function;
  prefixMethods: string[];
  postfixMethods: string[];
  transpilerMethods: string[];
}

/**
 * Singleton that collects all @SkyPatch class registrations.
 * Populated at module-load time when patch files are require()'d.
 */
export class PatchRegistry {
  private static instance: PatchRegistry | null = null;
  private entries: RawPatchEntry[] = [];

  private constructor() {}

  static getInstance(): PatchRegistry {
    if (!PatchRegistry.instance) {
      PatchRegistry.instance = new PatchRegistry();
    }
    return PatchRegistry.instance;
  }

  /** Called by the @SkyPatch class decorator. */
  addPatch(entry: RawPatchEntry): void {
    this.entries.push(entry);
  }

  /** Returns all registered raw entries (read-only view). */
  getEntries(): Readonly<RawPatchEntry[]> {
    return this.entries;
  }

  /**
   * Resolves all raw entries into RegisteredPatch objects by reading
   * each patch class's method bodies from the provided ts-morph source files.
   * Call this after all patch files have been require()'d.
   */
  resolveAll(_project: Project, patchSourceFiles: SourceFile[]): RegisteredPatch[] {
    return this.entries.map((entry) => resolveEntry(entry, patchSourceFiles));
  }
}

function resolveEntry(entry: RawPatchEntry, patchFiles: SourceFile[]): RegisteredPatch {
  const patchClassName = entry.constructor.name;

  // Find the class declaration in patch source files by class name.
  let classDecl = undefined as ReturnType<SourceFile["getClass"]> | undefined;
  for (const sf of patchFiles) {
    const found = sf.getClass(patchClassName);
    if (found) {
      classDecl = found;
      break;
    }
  }

  if (!classDecl) {
    throw new Error(
      `PatchRegistry: class "${patchClassName}" not found in any patch source file. ` +
      `Ensure the patch file exports a class with this exact name.`
    );
  }

  const patches: PatchInfo[] = [];

  for (const methodName of entry.prefixMethods) {
    const method = classDecl.getStaticMethod(methodName);
    if (!method) {
      throw new Error(
        `@Prefix method "${methodName}" not found as a static method in class "${patchClassName}"`
      );
    }
    patches.push(extractPrefixInfo(method));
  }

  for (const methodName of entry.postfixMethods) {
    const method = classDecl.getStaticMethod(methodName);
    if (!method) {
      throw new Error(
        `@Postfix method "${methodName}" not found as a static method in class "${patchClassName}"`
      );
    }
    patches.push(extractPostfixInfo(method));
  }

  for (const methodName of entry.transpilerMethods) {
    const fn = (entry.constructor as unknown as Record<string, unknown>)[methodName];
    if (typeof fn !== "function") {
      throw new Error(
        `@Transpiler method "${methodName}" is not callable on class "${patchClassName}"`
      );
    }
    patches.push({ kind: "transpiler", fn: fn as (method: MethodDeclaration) => void });
  }

  return { target: entry.target, patches };
}

function extractPrefixInfo(method: MethodDeclaration): PrefixPatchInfo {
  const params = method.getParameters();
  const paramNames = params.map((p) => p.getName());
  const paramTypes = params.map((p) => {
    const typeNode = p.getTypeNode();
    return typeNode ? typeNode.getText() : "any";
  });
  const returnTypeNode = method.getReturnTypeNode();
  const returnType = returnTypeNode ? returnTypeNode.getText() : "boolean";
  const bodyText = extractBodyText(method);
  return { kind: "prefix", bodyText, paramNames, paramTypes, returnType };
}

function extractPostfixInfo(method: MethodDeclaration): PostfixPatchInfo {
  const params = method.getParameters();
  const paramNames = params.map((p) => p.getName());
  const paramTypes = params.map((p) => {
    const typeNode = p.getTypeNode();
    return typeNode ? typeNode.getText() : "any";
  });
  const bodyText = extractBodyText(method);
  return { kind: "postfix", bodyText, paramNames, paramTypes };
}

/**
 * Extracts the statement-level text from inside a method body block,
 * suitable for inlining into an IIFE.
 */
function extractBodyText(method: MethodDeclaration): string {
  const body = method.getBody();
  if (!body) return "";
  const block = body.asKindOrThrow(SyntaxKind.Block);
  const statements = block.getStatements();
  if (statements.length === 0) return "";
  return statements.map((s) => s.getText()).join("\n");
}
