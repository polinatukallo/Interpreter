# ITMOScript Interpreter

An interpreter for a Lua-like programming language called ITMOScript.

## Overview

I implemented an interpreter for the ITMOScript language. This program can interpret source code files written in ITMOScript, similar to how Python works. The interpreter also supports an interactive REPL mode. Source files use the `.is` extension.

## Syntax

### Basic Rules

- Code is case-sensitive
- Whitespace and tabs can be inserted in any amount and don't affect execution (unlike Python)
- Comments use `//` and continue until the end of the line
- Expressions cannot span multiple lines

### Standard Data Types

1. **Numbers**
   - Signed values
   - Special boolean literals: `true` (1) and `false` (0)
   - Supports scientific notation (e.g., `1.23e-4`)
   - All numbers (including floating point) support arbitrary precision arithmetic

2. **Strings**
   - String literals are enclosed in double quotes `""`
   - Supports escape sequences
   - Correct syntax example: `"Some \"string\" type"`

3. **Lists**
   - Dynamic arrays
   - Literals in square brackets: `[1, 2, 3]`
   - Zero-based indexing
   - Supports slices

4. **Functions**

5. **NullType**
   - Special type representing nothing
   - Literal: `nil`

### Operators

1. **Arithmetic**
   - `+`, `-`, `*`, `/`, `%` (modulo)
   - `^` (exponentiation)
   - Unary `+` and `-`

2. **Comparison**
   - `==`, `!=`, `<`, `>`, `<=`, `>=`

3. **Logical**
   - `and`, `or`, `not`

4. **Assignment**
   - `=`, `+=`, `-=`, `*=`, `/=`, `%=`, `^=`

5. **Other**
   - `[]` (indexing)

### Conditional Statements
```
if condition then
    // expressions
elif condition then
    // expressions
else
    // expressions
end if
```
### Loops
```
while condition
    // expressions
end while
```
```
for i in sequence
    // expressions
end for
```
### Functions

Functions are first-class objects declared with:
```
name = function(parameters)
   // expressions
   return result
end function
```
Called with:
```function_name(arg1, arg2)```

## Standard Library

### Number Functions

- `abs(x)` - absolute value
- `ceil(x)` - round up
- `floor(x)` - round down
- `round(x)` - round to nearest integer
- `sqrt(x)` - square root
- `rnd(n)` - random integer from 0 to n-1
- `parse_num(s)` - string to number conversion
- `to_string(n)` - number to string conversion

### String Functions

- `len(s)` - string length
- `lower(s)` - to lowercase
- `upper(s)` - to uppercase
- `split(s, delim)` - split string
- `join(list, delim)` - join list to string
- `replace(s, old, new)` - replace substring

### System Functions

- `print(x)` - output without newline
- `println(x)` - output with newline
- `read()` - read input string
- `stacktrace()` - get call stack

## Implementation

- Dynamic typing
- Automatic memory management
- Lexical scoping
- Line-by-line interpretation
- Complete error handling
- Value/reference semantics like Python

## Examples

Example programs included:
- fibonacci.is
- maximum.is
- fuzzBuzz.is
