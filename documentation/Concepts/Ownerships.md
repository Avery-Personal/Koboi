# Ownership

Ownerships in Koboi are *loosely* based off the Rust ownership system. To start off with ownership, we first need to understand how they work before writing code for them. Ownerships, regardless of language's ownership system, is a memory system used to cleanly give & free memory by the main concepts of moving & borrowing data. Saying we have `Color`, alongside `FavoriteColor`, if `Color`, is `Blue`, we can do a simple move, using the assign operator, `=`, together as `Color = FavoriteColor`.

Now that we get the basics of ownership in Koboi, lets learn the memory movement methods, alongside what they apply for. Looking at all data types, the following, are ones that are **move** by default: Strings, Arrays, Files, Structs. All *other* data types, I.E integers, are **copy** by default.

The first ownership assignment operator we'll learn is `&`, used for borrowing values; borrowing values in Koboi allows for keeping the original owner of a value, allowing for multiple borrowings of values. Moving values for memory-specific data types, like the `FavoriteColor = Color` example, transfers ownership between values. The transferring of values, like said example, means `Color`, cannot be used in any context, & is now invalid.

The next ownership operator we'll learn is `@`, used for copying values. All non-memory based data types, I.E floats, are copy by default, meaning they copy the first value with the second value without transferring memory or borrowing of values. Using our original example, if we want to copy the value of `Color`, we can do `@`, followed by variable, in this case, `Color`, together as `FavoriteColor = @Color`.

Now that you know all basic usage of ownership & memory in Koboi, let's get into more advanced topics. To start, lets talk about a trailing variable. A trailing variable, is a Koboi specific assignment type, expressed with the `..` operator, *after* the variable. For example, if we have `Apples`, 5, & `Fruits`, and want to not constantly change the values of both, then you'd do a trailing assignment, allowing for reactive programming for variables. An example of usage, using said example, would be as so `Fruits = Apples..`; variables trailing another can NOT have their values changed, as their following a trailed value.

The next advanced type to talk about is the direct-memory transfer, expressed with the `#` operator, *before* a variable. The usage of this operator is a combo-operator, meaning it can be stacked on-top of other memory-assignment types. The usage of direct-memory transfer allows for mutable borrowing, `&`. Example of usage would be as so `FavoriteColor = #Color..`. If you don't understand the difference between standard movement & direct-memory transfers, I'd recommend looking into Assembly's `mov`/`load`.

Looking into our next advanced type, it's the multi-purpose `$` operator. During assignment with the usage of `$`, acts as a way to allow for optional ownership, meaning you can free the ownership on the variable; freeing variables via `$` retains the last value. To pair the `$` operator, you can use the `!` operator, needed for freeing variables in ownership with `$`, alongside needed if wanting to change ownership assignment, I.E Var1 being borrowed to Var1 being trailed. The usage of `$` during open programming, that is, non-assignment or defining, before the variable, acts as a way to free a variable, retaining the last value. Freed variables are able to have values of their own, but under a `linear` value, meaning it can only be used once after such. An example of such is as so:

```rs
fn main() {
    A = "Hello, World!"
    B = &A // Usage of standard borrowing

    $B // Frees variable, converted to linear if not already

    X = "Koboi is complex..."
    Z = !$X // ! used for explicit stating of freed variables, alongside changing of ownership types

    $X // Frees variable, converted to linear & eligble to be of another ownership

    X = #A..

    pln(X) // X is now used & invalid; linear values can only be used once
}
```
