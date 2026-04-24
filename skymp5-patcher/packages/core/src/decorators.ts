import "reflect-metadata";
import type { SkyPatchTarget } from "./types";
import { PatchRegistry } from "./registry";

// Metadata keys stored on patch class constructors by method decorators.
// Class decorators run after method decorators (bottom-up), so these are
// populated by the time @SkyPatch reads them.
const META_PREFIX_METHODS = Symbol("skypatch:prefix");
const META_POSTFIX_METHODS = Symbol("skypatch:postfix");
const META_TRANSPILER_METHODS = Symbol("skypatch:transpiler");

/**
 * Class decorator. Registers the patch class in the global PatchRegistry.
 *
 * Must be applied after @Prefix / @Postfix / @Transpiler method decorators
 * (i.e., placed outermost / closest to the class keyword).
 *
 * @example
 * @SkyPatch({ file: "systems/login.ts", class: "Login", method: "initAsync" })
 * class MyPatch { ... }
 */
export function SkyPatch(target: SkyPatchTarget): ClassDecorator {
  return function (constructor: Function): void {
    const prefixMethods: string[] =
      Reflect.getMetadata(META_PREFIX_METHODS, constructor) ?? [];
    const postfixMethods: string[] =
      Reflect.getMetadata(META_POSTFIX_METHODS, constructor) ?? [];
    const transpilerMethods: string[] =
      Reflect.getMetadata(META_TRANSPILER_METHODS, constructor) ?? [];

    PatchRegistry.getInstance().addPatch({
      target,
      constructor,
      prefixMethods,
      postfixMethods,
      transpilerMethods,
    });
  };
}

/**
 * Method decorator. Marks a static method as a prefix patch.
 *
 * The method runs before the original method body. Return false to skip
 * the original method entirely.
 *
 * Parameters: __instance (this of the target), then the original method params.
 * Return type: boolean
 */
export function Prefix(): MethodDecorator {
  return function (
    target: Object,
    propertyKey: string | symbol,
    _descriptor: PropertyDescriptor
  ): void {
    // For static methods, `target` is the constructor function.
    const ctor = target as Function;
    const existing: string[] = Reflect.getMetadata(META_PREFIX_METHODS, ctor) ?? [];
    Reflect.defineMetadata(META_PREFIX_METHODS, [...existing, String(propertyKey)], ctor);
  };
}

/**
 * Method decorator. Marks a static method as a postfix patch.
 *
 * The method runs after the original method body.
 * Parameters: __instance (this), original params, __result (return value).
 */
export function Postfix(): MethodDecorator {
  return function (
    target: Object,
    propertyKey: string | symbol,
    _descriptor: PropertyDescriptor
  ): void {
    const ctor = target as Function;
    const existing: string[] = Reflect.getMetadata(META_POSTFIX_METHODS, ctor) ?? [];
    Reflect.defineMetadata(META_POSTFIX_METHODS, [...existing, String(propertyKey)], ctor);
  };
}

/**
 * Method decorator. Marks a static method as a transpiler patch.
 *
 * The method receives the ts-morph MethodDeclaration and can perform
 * any direct AST manipulation. Runs before prefix/postfix injection.
 *
 * @example
 * @Transpiler()
 * static transpiler(method: MethodDeclaration): void {
 *   method.setBodyText("throw new Error('replaced');");
 * }
 */
export function Transpiler(): MethodDecorator {
  return function (
    target: Object,
    propertyKey: string | symbol,
    _descriptor: PropertyDescriptor
  ): void {
    const ctor = target as Function;
    const existing: string[] = Reflect.getMetadata(META_TRANSPILER_METHODS, ctor) ?? [];
    Reflect.defineMetadata(META_TRANSPILER_METHODS, [...existing, String(propertyKey)], ctor);
  };
}
