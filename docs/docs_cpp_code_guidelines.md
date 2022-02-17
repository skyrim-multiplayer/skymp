# Сpp Code Guidelines

## Language

The project uses standard C++17.

SkyrimPlatform and client code must be compilable with Microsoft Visual C++ 2019.
Server code and unit tests must be also compilable with Clang 12.

## Code Style

Code blocks require braces.

```c++
// ok
if (foo) {
  bar();
}

// not ok
if (foo) bar();

// also not ok
if (boo)
  bar();
```

## Naming

### Variables

The first word is lowercase, any other words must start with an uppercase: `camelCase`.

Global, static and thread-local variables must be prefixed with `g_`: `static int g_foo;`,
unless they are constants.

Constants should be prefixed with `k`. When declaring constants, please keep in mind that
[there can be side effects](#global-like-variables).

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

### Enums

Enum names must start with an upper case: `enum SomeEnum`. This rule is also applied to enum constants.

Declare enums as `enum class`, unless you have a really good reason to make them regular just `enum`.

### Functions

Functions must start with an upper case: `void LaunchOpenBeta();`.

Functions must start with a verb.

## Global-like variables

Don't make any global, static or thread-local variables or constants which have complex structure and/or destructors.
The destruction order of globals is undefined and can lead to unexpected bugs.

```c++
// ok
const KeyCode kDefaultChatHotkey = KeyCode::F6;  // enum
const std::string kDefaultName = "ThisIsAVeryOriginalUsername";
const std::unordered_map<std::string, std::string> kSomeMapping{
  { "a", "b" },
  { "b", "a" },
};

// not ok
struct WrappedRef {
  const std::string& something;
};
const WrappedRef kSomeWrappedRef{ kDefaultName };

{
  // ...
  // This is very dangerous and can lead to crashes. Don't do like this!
  // Existing places will be eventually removed.
  thread_local JsValue g_undefined = JsValue::Undefined();
}
```

## Const Qualifier

### Variables

Use `constexpr` when possible.

Declare class fields and methods as const when possible.

Rule of thumb for function arguments:

1. Do you want to change it outside of the function? `void Func(SomeType& outVar)`
2. Is it a primitive (`enum`, `int64_t`, `char`, etc.)? Pass as non-const value: `void Func(int64_t var)`
3. Do you want to use move semantics? Use `SomeType var` for copy+move and `SomeType&& var` for move-only
4. Otherwise, pass it as a const reference, e.g. `void Func(const std::string& message, const MyStruct& data)`

Do not mark local variables or function arguments passed by value as const.
The idea is that things with small scope do not need to be const.

Prefer const global/static/thread-local variables over non-const until you have a good reason making them non-const.

### Methods

Declare methods as const when possible.

Const methods must be thread-safe.

### Types

Prefer `int32_t` over `int`, `int16_t` over `short`, etc.

### Macros

`#endif` must be followed by a comment:

```c++
#ifdef _DEBUG
    auto debugFunctionJson = f.dump();
#endif // _DEBUG
```

Refrain from defining your own macros, unless required logic can't be implemented with C++ templates.
(But, even so, please think twice.) If you have to define a macro, `#undef` it as soon as it's not needed anymore.

Example:

```c++
#define DO_MAGIC(a, b, c) ...
DO_MAGIC(foo, bar, baz)
DO_MAGIC(fizz, buzz, fizzbuzz)
#undef DO_MAGIC
```

## Application to Existing Code

Project codebase may violate the rules above in some places. If you work with code that violates the rules consider fixing it.
