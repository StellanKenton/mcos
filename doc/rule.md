# Coding Rules

## 0. Environment Visual Studio Code + windows11

## 1. Comments: English only

## 2. Text Encoding & Editor Format
- Default text encoding: UTF-8
- Default indentation: 4 spaces, no tabs
- Default line ending: LF (`\n`)
- Trim trailing whitespace before commit
- Every text file must end with a single newline
- Exception: keep Markdown trailing spaces only when they are intentionally used for line breaks

## 3. Brace Style: Same line
```c
void func() {
}
if (x) {
} else {
}
```

## 4. General Engineering Principles
- Follow existing project conventions before introducing new patterns
- Do not mix refactoring with feature changes unless required by the task
- Every non-trivial change should consider error handling, logging, and testability
- Public behavior and interfaces must remain backward compatible unless the task explicitly allows breaking changes

## 5. Naming
- Use descriptive names;
- Use camelCase for identifiers in C code unless an external SDK or third-party library requires a different style
- Global variables and file-scope static variables must start with `g`, for example `gSystemState`
- Temporary local variables should start with `l`, for example `lRetryCount`
- Struct type names should start with `st`, for example `stWifiConfig`
- Enum type names should start with `e`, for example `eSystemMode`
- Function names should start with the module name whenever practical, for example `uartInit`, `wifiConnect`, or `motorSetSpeed`
- Macro names should use all caps with underscores only when a macro is necessary
- Constants should follow the surrounding module style; prefer `const` objects over macros when C allows it cleanly
- Boolean names should read as predicates, such as `isReady`, `hasAlarm`, or `shouldRetry`

## 6. File and Dependency Rules
- Keep one clear responsibility per file
- Prefer small headers and small modules over large multi-purpose files
- Add new dependencies only when they provide clear value
- Include or import only what is used
- Remove dead code, unused includes, and unused imports introduced by the change

## 7. C Rules

### 7.1 Style and Language Use
- Follow the C standard configured by the project toolchain; do not require a newer compiler mode without need
- Write portable C first, then add compiler-specific extensions only when they are required and isolated
- Use `const` wherever it improves correctness, especially for lookup tables, pointer parameters, and read-only buffers
- Initialize variables at declaration whenever practical; do not leave state implicit
- Prefer explicit casts only when necessary; do not use casts to hide type mistakes or warnings
- Prefer `static inline` helper functions over function-like macros when type safety and readability matter
- Use `NULL` only for null pointers; do not use `0` to express pointer intent

### 7.2 Module and File Design
- One source file should implement one clear module responsibility
- Expose only the minimum required interface in headers; keep internal helpers `static`
- Functions in the same module should use a consistent module prefix
- Avoid cross-module access to internal state; provide access functions when coordination is needed
- Keep header files small, stable, and safe to include from multiple translation units

### 7.3 Data Types and Memory
- Use fixed-width integer types such as `uint8_t` and `int32_t` when register layout, protocol layout, storage width, or overflow boundaries matter
- Use plain `int` only when width does not matter and it matches the surrounding codebase convention
- Keep struct layout intentional; do not rely on compiler packing unless the hardware or protocol requires it and the tradeoff is documented
- Avoid dynamic allocation in embedded runtime paths unless the module clearly owns the memory model and failure handling
- If dynamic allocation is necessary, define allocation, release, timeout, and failure behavior explicitly
- Prefer caller-provided buffers in hot paths and ISR-adjacent code

### 7.4 Functions and Parameters
- Keep functions short, single-purpose, and easy to trace in a debugger
- Validate pointer parameters, sizes, and enum values at module boundaries
- Return explicit status codes when failure is possible; document the meaning of each return value
- Use output parameters only when they are clearer than returning a value directly
- Avoid long parameter lists; group related parameters into a struct when they represent one configuration object
- Do not hide hardware side effects in utility-looking functions

### 7.5 Macros, Enums, and Structs
- Use macros only for compile-time constants, conditional compilation, small token manipulation, or hardware register access patterns that cannot be expressed well in C
- Parenthesize macro parameters and the full macro result when arithmetic or expressions are involved
- Do not place side effects in macro arguments unless the macro is explicitly designed for that contract
- Prefer enums for named state sets and mode selections; keep enum values stable when they map to protocol or hardware values
- Use struct names with the `st` prefix and enum names with the `e` prefix to keep type intent obvious in mixed embedded code

### 7.6 Headers and Includes
- Headers should compile cleanly when included in a source file on their own with required dependencies present
- Use include guards or `#pragma once` consistently with the existing project style
- Include order: related header, standard headers, third-party headers, project headers
- Do not include heavy or unrelated headers from other headers unless the interface requires them
- Place shared declarations in headers and keep definitions in source files unless `static inline` is justified

### 7.7 Error Handling and Safety
- Use the project's established error-handling model consistently across modules
- Never ignore hardware, communication, or allocation failures silently
- Assertions can protect programmer assumptions, but they must not replace runtime error handling for expected failures
- Keep log output actionable and low-noise, especially in fast loops, interrupt-adjacent paths, and communication handlers
- Mark interrupt-shared or hardware-updated state `volatile` only when the access model truly requires it; do not use `volatile` as a synchronization substitute

### 7.8 Concurrency and Interrupt Context
- Minimize shared mutable state between tasks, interrupts, and callbacks
- Keep ISR code short, bounded, and free of blocking operations
- When data is shared across contexts, make ownership, update order, and critical-section rules explicit
- Prefer lock-free or deferred-processing patterns for interrupt-heavy paths when they reduce latency and complexity
- Document which APIs may be called from task context, interrupt context, or both

### 7.9 Testing
- Add or update tests for non-trivial logic changes when the target platform or host test environment supports it
- Prefer deterministic tests over timing-sensitive tests
- Cover normal flow, boundary conditions, invalid inputs, and failure paths
- For hardware-facing code, isolate pure logic so it can be tested without the full device when practical

## 8. Python Rules

### 8.1 Style and Structure
- Target the Python version used by the project environment
- Follow PEP 8 unless the repository already enforces a different formatter or linter style
- Use 4 spaces for indentation and keep files UTF-8 encoded
- Prefer clear, direct code over compact one-liners
- Keep modules focused; separate scripts, reusable libraries, and configuration logic

### 8.2 Typing and Interfaces
- Add type hints to new public functions, methods, and module-level constants where practical
- Public functions should have clear inputs, outputs, and error behavior
- Use `dataclass` when it improves clarity for structured data objects
- Prefer `Enum` for discrete sets of named values
- Avoid returning multiple unrelated meanings from one value; use explicit result objects when needed

### 8.3 Imports and Dependencies
- Group imports as standard library, third-party, and local modules
- Avoid wildcard imports
- Import only what is used
- Prefer `pathlib.Path` over manual string path manipulation
- Add external packages only when the standard library or existing dependencies are insufficient

### 8.4 Function and Class Design
- Keep functions small and cohesive
- Avoid hidden side effects
- Prefer pure functions for transform logic when possible
- Use classes only when state and behavior naturally belong together
- Keep constructors lightweight; avoid heavy IO or network calls in `__init__`
- Use context managers for files, locks, and other managed resources

### 8.5 Error Handling and Logging
- Catch specific exceptions, never use bare `except:`
- Raise exceptions with messages that help locate and diagnose the problem
- Do not suppress exceptions unless there is a clear recovery strategy
- Use `logging` instead of `print` for non-trivial runtime diagnostics
- Validate external inputs early

### 8.6 Python Pitfalls to Avoid
- Do not use mutable default arguments
- Do not rely on implicit truthiness when it hides domain meaning
- Avoid dynamic attribute creation unless the pattern is deliberate and documented
- Avoid large blocks of top-level executable code; use functions and `if __name__ == "__main__":` for scripts
- Prefer comprehensions only when they remain readable

### 8.7 Testing
- Add or update tests for non-trivial Python logic
- Prefer `pytest` style if the repository already uses it; otherwise follow the local test framework
- Test boundary conditions and invalid inputs, not only happy paths

## 9. AI Execution Rules
- Before writing code, inspect nearby files and follow the dominant local style
- Preserve existing architecture unless the task requires structural change
- Do not introduce speculative abstractions, premature optimization, or unnecessary indirection
- When modifying code, also update tests, documentation, or configuration if they are directly affected
- Do not rewrite large sections just to satisfy style preferences
- Prefer the smallest correct change that fully solves the problem
- If a rule conflicts with the existing codebase convention, follow the existing codebase convention

## 10. Output Quality Standard for AI
- Generated code must compile or run in principle within the project's toolchain and dependency set
- Generated code must be complete enough to integrate directly into the repository
- Generated code must avoid placeholders such as `TODO`, `fixme`, or pseudo-code unless the user explicitly requests a draft
- Generated code should include concise comments only where the intent is not obvious from the code itself

