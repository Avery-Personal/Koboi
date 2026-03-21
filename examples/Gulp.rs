^![M<SFM>]^ // ^^ - Compiler Flag | ! - Full File | [] - Sector | M - Mandatory | <> - Feature constant | SFM - Safe mode (Compiler flag indicating for file to stay on safe mode constantly, overriding all safety mode calls for the full file & is mandatory)
!^^A^ // ! - Cancel | ^ .. ^ - Compiler Flag | ^ - Flag | A - Above (Cancel the compiler flag above [! first means cancel, inside a compiler flag indicates full file])

using std::io // Used for pln (Print Line) macro & EXIT([EXIT_CODE])
using std::memory // Used for complete([DATA]) (Used to determine if a partial is complete or not)

x = 5 // Scope level inferred variable
int xx = 7 // Declaration-first, integer variable
xxx : int = 9 // Constraint integer variable

global y = 5.5 // Globalized variable
silent yy = 2.75 // Silented variable (Only callable by its scope [Can't be called by child scopes], dies after new scope is met OR used on a value) | IN WORKS, NOT USED AON
private yyy = 12345 // Private variable (Private variable that cannot be called into call types [functions], child scopes, etc.) | IN WORKS, NOT USED AON
export yyyy = 123.45 // Exported variable (Public data outside of modules)

const READ = "Don't edit me." // Constant Variable
static Calls = 0 // Static variable

linear z = 190226 // Linear variable (Variable that can only be used once)
history zz = 42 // History variable (Used to allow for value tracking)
symbolic zzz : int // Symbolic variable (Runs code with symbolic values rather then real ones) | NOT USED AON

module Counter {
    static Count = 0

    export fn Increment() {
        Count += 1
    }
}

struct User {
    Name : String
    Age : int
}

// Partial initialization allowing for handling incomplete data safely
partial UserSafe {
    Name : String
    Age : int
}

env ID // Environment variable
/*

Capability & Context variable

Creating the environment, as so, above; the 'capability'.
An entry point or scope must provide the environment to put it in effect, I.E:

fn main() provides env ID = 1 { .. }

An environment can only be used via its scope providing it, so in the usage case of lets say I try doing:

fn InitializeEnvironments() provides env Filesystem provides env ID = 1 {return}

Syntactically it would work as it's providing the environment, but due to the context, it wouldn't work outside of the scope.

When having a function that needs an environment, I.E saying an 'IDLog', we'd do it like so:

fn IDLog() requires env ID { ... }

Requiring for the environment gives the information/data of the environment's use in the scope calling the function, I.E

fn main() provides env ID = 1 {
    IDLog()
}

IDLog would get the environment's data, the ID, of 1, as that's the scope that used the environment that called the function requiring it.

With these use cases, an environment can be in the usage of permission or data, I.E having an environment, lets say Filesystem, and we want to have a delete function, we can make a safety for it via:

fn DeleteFile(Path : String) requires env Filesystem {
    DeleteFunction( ... )
}

If we do a non-environmented call of DeleteFile, lets say main, not providing Filesystem, then the program would result in a compile-time error.

To create information in environments, it's similar usage of macros in Koboi, after defining the environment, in said case, ID, we can use braces to form a body, env ID {}.
After creating a body, we're able to allow for simple data such as variable & basic functions; functions are NOT able to use advanced types, I.E unsafe functions.
Due to functions being directly in an environment, the use case of 'requires', is not mandatory, & is handled by the compiler. An example of such use case is so:

env ID {
    IDNumber : int

    fn CreateID() {
        IDNumber = 3
    }
}

*/

sys DEBUG = 0 // System, compile-time environment | 0 - False, needs std::core/std::types to get it but want to show pure Koboi (outside of pln & exit).

if sys.DEBUG
    print("Debugging: ON\n")
else
    print("Debugging: OFF\n")

// Combo example
global export int xy = 8

a = "Hello, World!"
b = a // A is now invalid due to ownership

print(a) // Error

a2 = "Hello, World!"
b2 = &a2 // BORROW a2 value, cannot edit a2 due to such

a2 = "Hi, World!" // Error

a3 = "Hello, World!"
b3 = @a3 // COPY a3 value, allows for editing of a3 while not editing b3's value too

a3 = "Hi, World!" // Valid :D

a4 = "Hello, World!"
b4 = a4.. // TRAILING a4 value, allows for editing of a4, but edits all values also using it

a4 = "Hi, World!" // Valid, changed b4 to such too
b4 = "Hello, World!" // Error, usage of trailing owner a4

a5 = "Hello, World!"
b5 = #a5 // # Uses the DIRECT memory address of a5, followed up by any ownership symbol wanted; allows for mutable borrowing.

// Allows for memory to be shared in a region, after region closes all memory is automatically freed.
region Memory {
    a6 = "Hello, World!"
    b6 = &a6
}

a7 = 0
b7 = 0

transaction {
    a7 += 1
    b7 += 2

    if b7 < a7 {
        rollback
    }
}

int c = 5
int v = 7

if v > c {
    pln("V is greater then C")

    c += 1

    if v > c {
        pln("V is still greater then C")
    }
}

for i=0, i < 5, i += 1 {
    pln(i)
}

while sys.DEBUG {
    static iteration = 0

    if iteration >= 50 {
        break
    }
}

enum Result {
    OK,
    ERROR
}

state FileState {
    Closed,
    Open
}

macro SetX(Value) {
    expose x = Value
}

macro repeat(LOOPS, BODY) {
    int __ITERATION__ = 0

    while __ITERATION__ < LOOPS {
        BODY

        __ITERATION__ += 1
    }
}

Result _Result = OK

IntegerList : [int, 3] = {2, 9, 42}
SlicedFloatList : [float] = {8.4, 9.6, 12.52, 123.45}
Point : [int, 2] = {2, 7}

ArrayCall = 2

x = 5

SetX(15)

pln(x) // 15

// Basic matching
match x {
    3 >> pln("3"),
    1 >> pln("1"),

    v >> pln("Other")
}

// Exhaustive matching
match _Result {
    OK >> pln("Expected result")
    // Missing ERROR, would error
}

// Pattern matching
match Point {
    (0, 0) >> pln("Origin"),
    (x, 0) >> pln("X-axis:", x),
    (0, y) >> pln("Y-axis:", y),
    (x, y) >> pln("Point:", x, y)
}

// Guard (w/ pattern) matching
match x {
    Value if Value > 0 >> pln("Positive"),
    Value if Value < 0 >> pln("Positive"),

    v >> pln("Zero")
}

// Body matching
match x {
    None >> EXIT(1)
    
    v >> {
        check {
            x != None
        }

        x = 10
    }
}

concept {
    // IN WORKS
}

context Release {
    Koboi = "Amazing"
}

context Development {
    Koboi = "Meh"
}

with Release {
    pln(Koboi)
}

reactive {
    al = 14
    l = a * 2
}

al = 10

pln(l) // 20

zz = 5

pln(zz@0) // 42
pln(zz@1) // 5

if zz > 5 {
    zz = 2 // Path B (Path passed)
} else {
    zz = 9 // Path A (Path gone down)
}

pln(zz@2b0) // 9
pln(zz@2) // Error (Multiple branches)

env filesystem // Standard environment (Gives access to File data type, FileOpen/FileWrite/FileDelete)

// Checkings mode allows for extensive checks via the compiler & runtime; simple conditions, I.E x > 1 is a simple check that can be made via the compiler, as it can simply check the value of x, then compare it with 1, harder, runtime-based features, I.E x > RandomNumber(1, 5), is passed through the compiler, if it cannot perform an analysis check to prove the check, then it passes it onto the runtime, to check. When a condition is proven false, via the compiler, then it will result in a compile-time error, via runtime, will produce a runtime-error & stop the program.
check {
    x != c
}

unsafe {
    pln(IntegerList[5])

    check {
        x < 4 // In checkings mode
    }

    safe {
        pln(IntegerList[4]) // Error, back in safe mode; safe is the default a program runs
    }
}

// Trusted unsafe allows for everything unsafe has to offer with the acceptance of environment bypassing, this is the one & ONLY case of which such can happen.
trusted unsafe {
    global UFILE = OpenFile("a.txt")

    pln(UFILE.source)
}

// Code that runs regardless of runtime errors
defer {
    CloseFile(UFILE)
}

// Assuming allows for you to give the compiler information, this allows for little to no checks on what you gave in, performance increases, & optimization.
// In shown example, we tell the compiler to assume that the ArrayCall's value is LESS then the IntegerList's length, the next call we do is using the IntegerList, and calling the index of which ArrayCall is used, the compiler uses knowledge from states & information fed into it to assume that what the user said is correct, so it will NOT do any checks to confirm if the length is less then the index value.
assume {
    ArrayCall < IntegerList.len
}

pln(IntegerList[ArrayCall])

// Functions are dynamic, just use the 'fn' keyword, and it automatically knows what to return; static types aren't needed, good for explicit function though
fn Add(a : int, b : int) {
    return a + b
}

fn GenerateTable() {
    return {1, 3, 7, 2}
}

// Compile time execution
compiletime {
    Table = GenerateTable()
}

context Debug {
    fn Log(Message) {
        pln("[DEBUG]", Message)
    }
}

context Release {
    fn Log(Message) {
        pln(Message)
    }
}

with Release {
    Log("Hello, World!")
}

// noalias prevents overlapping memory
fn Modify(a : noalias [int], b : noalias [int]) {
    a[0] = 10;
    b[0] = 20;
}

fn PrintLoop(LOOPS : int) {
    repeat(7, {pln(LOOPS)})
}

unsafe fn PrintArray(Array : [int], Index : int) {
    pln(Array[Index])
}

compiletime fn factorial(n: int) {
    if n <= 1 return 1
    
    return n * factorial(n - 1)
}

fn GetFile() requires env Filesystem {
    return OpenFile("a.txt")
}

fn main() provides env Filesystem {
    FILE : File = GetFile()
    
    Sum = Add(3, 7)

    print(Sum)
}

Number = 5

// Isolated programs allowing for the use of multiple programs in  one, large, program.
world FirstProgram {
    fn main() {
        Number = 8 // Local modification

        pln(Number)
    }
}

world SecondProgram {
    fn main() {
        pln(Number) // 5
    }
}

world Base {
    k = 3

    fn Setup() {
        pln("Base setup")
    }
}

// Basic inheritance
world Derived extends Base {
    fn main() {
        Setup()

        pln(k)
    }
}

// Basic overriding
world Base2 {
    k = 5

    fn Greeting() {
        pln("Hello, World!")
    }
}

world Derived2 extends Base2 {
    k = 10

    fn Greeting() {
        pln("Hi, World.")
    }
}

// Cross-inheritance
world A {
    k = 1
}
world B extends A {
    k = 2
}

world C extends B {
    pln(k) // 2
}

// Language implementation (Rebekah, Naomi, & Avi-Assembyl)
nao {
    reg r1
    reg r2

    main:
        r1 = 5
        r2 = 3

        r1 = r1 * r2

        halt
}
