# Сpp Code Guidelines

## Language

The project uses standard C++17.

## Code Style

Code blocks require braces.
```c++
// ok
if (foo) {
  bar();
}
// not ok
if (foo) bar();
```

## Naming

### Variables

The first word is lowercase, any other words must start with an uppercase: `camelCase`.

Global, static and thread-local variables must be prefixed with `g_`: `static int g_foo;`

The prefix is should be used for boolean variables and methods (ex: `isSet`, `isFinished`, `isVisible`, `isFound`, `isOpen`).

There are a few alternatives to the is prefix that fit better in some situations. These are the has, can and should prefixes (ex: `hasLicense`, `canEvaluate`, `shouldSort`).

Constructor parameters have an underscore at the end if there is a member variable with the same name:

```c++
class Foo {
public:
  Foo(int bar_);
  const int bar;
};

Foo::Foo(int bar_) : bar(bar_) {};
```

Do not use short variable names. Prefer `server` over `svr`.

Short names that are ok: `i`, `n`, `it` in loops; `lhs`/`rhs` in operator overloading; `res` for function result.

### Classes

Class names must start with an upper case: `class SomeClass`. 

No I-prefix is used to indicate abstract classes. 

Class name should be a non-verb noun whenever possible.

### Functions

Functions must start with an upper case: `void LaunchOpenBeta();`.

Functions must start with a verb.

## Const Qualifier

### Variables

Use `constexpr` when possible.

Declare class variables as const when possible. Do not declare function arguments or local variables as const. The idea is that things with small scope do not need to be const.

Prefer const global/static/thread-local variables over non-const until you have a good reason making them non-const.

### Methods

Declare methods as const when possible.

Const methods must be thread-safe.

### Types

Prefer `int32_t` over `int`, `int16_t` over `short`, etc.

## Application to Existing Code

Project codebase may violate the rules above in some places. If you work with code that violates the rules consider fixing it.
