# ras

ras is a runtime assembler for the aarch64 architecture written in C
with an assembly like syntax provided by preprocessor macros.
Currently it supports most of the common parts of the integer 
instruction set useful for runtime code.

## Usage

Inside the `ras` directory are 3 files. `ras.h` contains
the public api and is also used to access the other files.
`ras_impl.h` contains the private implementation and is accessed
by defining `RAS_IMPL` before including `ras.h`. `ras_macros.h`
contains the macro api and is enabled by defining `RAS_MACROS`
before including `ras.h`. The macro api should not be enabled
in a header file. You need to enable `RAS_IMPL` in an implementation
file.

Note: the macro api makes heavy use of `_Generic` and `__VA_OPT__`
so you will need to use a compiler or C standard that supports those.

There are usage examples in the `examples` directory.

There are some options you can enable with `#define` 
before including the implementation:
|  |  |
| - | - |
| `RAS_AUTOGROW` | enable automatically resizing code |
| `RAS_NO_CHECKS` | disable all asserts |
| `RAS_USE_RWX` | use rwx memory for code (default switches between rw and rx) |

There are also options for the macro api:
|  |  |
| ------ | ---- |
| `RAS_DEFAULT_SUFFIX` | set this to either `w` or `x` for default register size |
| `RAS_CTX_VAR` | set this to the name of the `rasBlock` variable you are using |

The syntax is a bit different from standard syntax: instead of using
`wN`/`xN` to specify registers, all GPRs are specified by `rN` and
the size is specified by a `w` or `x` suffix to the instruction.
A default suffix can be optionally enabled. There are examples
for every instruction in `tests/test_input.txt`.
