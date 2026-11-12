# Project Instructions

## Platform and Changes
- Follow the target, framework, versions, and conventions established by the project.
- If an implementation depends on target-specific features or hardware capabilities (e.g. PSRAM), ask before proceeding with a dependent implementation.
- Do not replace existing libraries, APIs, or implementation approaches without explicit approval.
- Do not change existing code without confirmation.
- Do not delete files or code, or create commits, without explicit approval.

## Naming Conventions
- `I` prefix: abstract types / interfaces (e.g. `IWidget`).
- `__` prefix: internal/system methods not intended for direct user calls (e.g. `__tick()`).
- `_` + `snake_case`: global objects or mutable class fields (e.g. `_fs`, `_menu`).
- `_` + `UPPER_SNAKE_CASE`: `const` (immutable) class member fields initialized via constructor (e.g. `_LENGTH`, `_RES_TYPE`).
- `UPPER_SNAKE_CASE`: macros, `enum` values, and compile-time constants (e.g. `UI_WIDTH`).
- `UpperCamelCase`: concrete types (classes, structs, enums).
- `lowerCamelCase`: public and protected methods.
- `snake_case`: local variables, function parameters, and plain struct fields.
- Use clear, conventional, self-explanatory names familiar to programmers.

## Code Quality and Resources
- Use Arduino's `delay()` instead of FreeRTOS's `vTaskDelay()`.
- Do not use `std::string`; use Arduino's `String` class instead.
- Optimize stack and RAM usage in performance- or memory-critical code; avoid premature or excessive optimization elsewhere.
- Preserve compiler optimizations; avoid changes that inhibit inlining, optimization, or efficient code generation without a clear need.
- For large data objects (e.g. images), use PSRAM when available, checking `psramInit()` before `ps_malloc()` and falling back to `malloc()` otherwise. Ask before proceeding if the target's PSRAM support or allocation strategy is uncertain.
- Use flash storage sensibly without excessive size-driven optimization.
- Avoid duplicated code by extracting genuinely reusable logic into functions; do not create trivial functions.
- Add comments only where code is difficult to understand. Document methods only, following the project's existing documentation style. Keep all other code self-explanatory.
- Check return values and handle errors where necessary; do not silently ignore failures.
- Do not use `static` variables or global variables declared outside classes, including local `static` variables and static class data members.
- Declare constants as `static constexpr` or `static const` in the `.cpp` file, immediately after the `#include` directives. Do not expose constants in headers without explicit approval.
- Avoid macros. If a macro may improve performance or is otherwise necessary, explain why and ask for approval before using it.
- Minimize class and struct padding by ordering fields according to their sizes and alignment requirements. Access specifiers (`private`, `protected`, `public`) may be repeated and interleaved as needed, without changing member accessibility.
- Never reorder or alter the layout of structs defined by external APIs, protocols, binary formats, or file specifications (e.g. image headers). For structs that must match an externally specified packed binary layout, use `#pragma pack(push, 1)` before the definition and `#pragma pack(pop)` after it.
- Do not use `#pragma pack` for ordinary project structs unless explicitly required by an external specification.

## Workflow
- Inspect relevant code before proposing changes.
- Make the smallest change needed to fulfill the request.
- Do not build, run tests, or write tests.

## Restricted Directories
- Do not inspect, read, search, modify, create, or delete anything inside `.pio/`, `.git/`, `.vscode/`, `.cache/`, or `build/` if these directories exist.
- Exclude these directories from file searches, code analysis, and automated operations.
- Do not access their contents for any reason unless explicitly authorized by the user.
