# Environments

## Understanding of permissions & capabilities

To start off environments, first you'll have to understand permissions in Koboi. Koboi is a very authoritive language, meaning it has a strict rule-based system for each part of Koboi, I.E ownership. To start with the learning of permissions, you'll need to understand how a permission works, we'll use our safety modes system to start easy. Looking at our safety modes, we have 5 total, most extensive being checkings mode, with the easiest being the trusted unsafe mode; when writing information, like conditions into checkings mode, the permissions, also known as capabilities in Koboi, as what environments are, a capability system, the permissions of what you're changing are changed via the mode.

Looking into trusted unsafe mode, nearly all permissions, or capabilities, of what the compiler enforces dissapears, outside of mandatory semantics analysis, I.E type matching. Trusted unsafe mode, is also, the one & ONLY mode of which environments can be bypassed, but is NOT recommended, due to the point of environments to start, unless explicitly wanted.

## Getting into environments

Now that we know the basics of permissions, capabilities, in Koboi, lets get into environments. Environments in Koboi are ways to create a context of information, alongside capabilities of such. The context of what environments allow is to let the calls of data without needing to explicitly pass in information. On a capability viewpoint, environments allow for systems to work seamlessly with each other via providation & requiring.

Environments are not limited to users' creation of environments; Koboi has standard environments, systems native to Koboi in an environment, most common of such being: `filesystem`, `time`, `process`. The usage of standard environments allows for safety of potentially dangerous use cases of libraries, I.E `filesystem` can be used to open, write, delete files, etc.

## Writing Koboi environments

To start off with *writing* environments in Koboi, you first need to know the following keywords: `env`, `provides`, `requires`. The `env` keyword is used to define an environment, alongside indicate your provided/required information **is** an environment. To provide & actually create an environment, you use the `provides` keyword in the scope of which you're to call a function needing said environment; example of such like so:

```rs
env filesystem

fn main() provides env filesystem {
    FILE : File = OpenFile("a.txt")
}
```

In shown example, we use our standard environment, filesystem; functions as such, like `OpenFile` have the requirements of filesystem in them. The `env filesystem` at the top of the file is defining the environment, the standard environment, filesystem. Our next step is providing our environment into a scope that will call & require said environment, in this case, `OpenFile`.

The last keyword, not yet expressed, is the `requires` keyword. The `requires` keyword is used in the scope of a program that uses data, functions, or information of an environment. Using any of such, data, functions, or information, without the providation or requirement with providation of an environment, results to a compile-time error. When providing an environment, you're also allowed to set the value of one, via the assignment operator, followed up by a value. An example of such would be so:

```rs
using std::io

env process
env ExitID

fn Exit() requires env process requires env ExitID {
    ExitProgram("zhs")
    EXIT(ExitID)
}

fn RunTerminal() requires env process {
    RunProgram("zhs")
}

fn main() provides env process provides env ExitID = 1 {
    RunTerminal()
    Exit()
}
```

## Writing information to environments

Now that you understand the basics of environments & can use standard environments, alongside writing your own in full, lets get into providing information to environments themselves. When creating an environment, you can create a body, body of such expressed by `{ ... }`, after defining the environment. The information of environments are extended to values, alongside basic functions, that of such does not include unsafe functions; advanced systems of Koboi, I.E safety modes, are not available in environment information, as it's purely provided data, not runtime processes, similar to macros in Koboi. An example of such is like so:

> Creation of function in an environment does NOT need the `requires` keyword, compiler handles automatically.
```rs
using std::random

env User {
    ID : int

    fn CreateID() {
        ID = RandomInt(1, 190226)

        return ID
    }
}

fn main() provides env User {
    UserID = CreateID()
}
```