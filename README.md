# Koboi

This is the main source code repository for [Koboi], containing the compiler, standard libararies, & documentation.

# What is Koboi?

Koboi is a medium-level programming language made & developed by **AveriC**, designed for absolute safety & control.

# Why Koboi?

Koboi is a refined language built around the idea of control; it's ecosystem is vast, each split into four sections: memory, handled by ownership; permissions, handled by environments; compiler checks, handled by safety modes; and verification, handled by checkings.

The Koboi compiler has 5 safety modes, each with a different level:

`check` - Extra compiler check & runtime verification
`safe` - Default compiler
`assume` - Allows for feeding compiler information for increased performance & optimization
`unsafe` - Turns off most compiler checks & allows for out of bounds programming (I.E outer array index)
`trusted unsafe` - Turns off **all** compiler checks outside of semantic data validation

# Core Features

## Environments

Environments in Koboi are ways to make a permission & capability. Creating an environment uses the `env` keyword, followed by the name of the environment, I.E `env AuthID`. When wanting to use an environment into a function, you use `provide env`, followed by the name of the enviornment, I.E `provide env AuthID`. To have a function that requires an environment, let's say `VerifyID`, we need to require the environment. To require an environment, you say `require env`, followed by the enviornment, I.E `requires env AuthID`.

For a more in-depth usage, understanding, & explanation of environments, refer to the [Environments](documentation/Concepts/Environments.md) page.

## Safety Modes

The Koboi compiler has 5 safety modes, as described in the [Why](#why-koboi) section of Koboi. Each having its own level of checks.

The `safe` mode on Koboi, as expressed by `safe` keyword, is the default staate of the Koboi compiler, running normal checks, semantics, etc.

The `checkings` mode on Koboi, expressed by `check` keyword, is the safest mode the compiler can be in, running extensive checks, and doing double checks of conditions via the compilation time & runtime.

The `assume` mode on Koboi, as expressed by `assume` keyword, is a way to pass information to the compiler to ensure an upcoming piece of data is true, used in cases of optimization & performance boosts.

The `unsafe` mode on Koboi, as expressed by `unsafe` keyword, is the second most unsafe mode on the compiler, running no compiler-based checks outside of mandatory semantic checks, I.E data type.

The `trusted unsafe` mode on Koboi, as expressed by `trusted unsafe` keyword(s), is the most unsafe mode on the compiler, running no compiler-based checks outside of mandatory semantic checks, I.E data types; environments are allowed to be bypassed via this mode, and this mode only.

# Example Programs

## Hello World

```rs
fn main() {
    print("Hello, World!")
}
```

## Counter

```rs
using std::io

module Counter {
    export static Count = 0

    export fn Increment() {
        Count += 1
    }
}

fn main() {
    while Counter::Count < 5 {
        pln("Count:", Counter::Count)

        Counter::Increment()
    }
}
```

## Matches

```rs
using std::io

enum Result {
    OK,
    ERROR,

    OTHER
}

Result __RESULT__ = OK

fn main() {
    Point : [int, 2] = {2, 7}

    match Point {
        (0, 0) >> pln("Origin"),
        (x, 0) >> pln("X-axis:", x),
        (0, y) >> pln("Y-axis:", y),
        (x, y) >> pln("Point:", x, y)
    }

    match __RESULT__ {
        OK >> pln("Expected result"),
        ERROR >> pln("Unexpected result"),

        v >> pln("Unknown result")
    }
}
```
