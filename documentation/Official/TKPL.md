# The Koboi Programming Language
## Version - 0.2b21 | GitHub Exclusive

> The TKPL (The Koboi Programming Language) is the official **manual** of Koboi.

---

## Table of Contents
### I. An Introduction to Koboi
#### 1.0.0 Core Language
##### 1.1.0 Installing KoboiC
##### 1.2.0 Installing Pako
###### 1.2.1 What is Pako
###### 1.2.2 Using Pako
###### 1.2.3 Creating a project
##### 1.3.0 Getting Started
###### 1.3.1 Creating your first program
###### 1.3.2 Project structure
###### 1.3.3 Running & Building
###### 1.3.4 Standard Libraries
##### 1.4.0 Basics
###### 1.4.1 Initializing variables
###### 1.4.2 Implicit vs Explicit
###### 1.4.3 Data Types

---

# Part I. An Introduction To Koboi

This section of the manual teaches the core language, alongside project structure, compilation, compiler usage, & Paki, the package manager for Koboi. If you want to go directly to learning the language, click [here](#150-basics-1). Without further ado, lets learn Koboi.

## 1.0.0 Core Language
### 1.1.0 Installing KoboiC

To install the official Koboi compiler, KoboiC, go to [Koboi-Language](https://Koboi-Language.github.io), or you can install the release/source via [GitHub](https://github.com/Avery-Personal/Koboi).

KoboiC, is the official compiler for Koboi, & used to run all source code written in Koboi; to run source code in Koboi, you call the compiler, `KoboiC`, followed by the file(s). For example, if we have `main.kb`, we call `KoboiC main.kb`.

### 1.2.0 Installing Pako

Do note, official downloads of Koboi (Standard libraries, compiler, etc.,) have Pako, including other manager(s) installed by default. If not so, install it via [Koboi-Language](https://Koboi-Language.github.io) or via a command install.

#### 1.2.1 What is Pako

Pako is the official project & package manager for Koboi. Pako is designed to make creating projects in Koboi easy & uses [Sendo](#130-installing-sendo-1) overhead for building projects.

#### 1.2.2 Using Pako

To use the Pako manager, you can simply use call the manager, `Pako`, followed by a command. A basic command we can do is `Pako execute`, used to execute source, I.E `main.kb`, together as `Pako execute main.kb`.

#### 1.2.3 Create a project

One of the main usages of Pako is creating projects, to bundle source for Koboi in one easy, to compile project. To create a project, we can call `Pako new`, followed by the name of the project, `Pako new <PROJECT_NAME>`.

### 1.3.0 Getting Started
#### 1.3.1 Creating your first program

The Koboi programming language uses the `.kb` file format. Examples of all basic-intermediate examples that are single-file or standard library dependencies will use KoboiC. The Koboi programming language uses `main`, to start off a program, but simple examples will purely show dumps.

One of the only native functions in Koboi is `print`, although `pln` via the **IO** library is standard. To make a function, needed to initialize the entry point, `main`, is `fn`, for function, followed up by the name of the function. Arguments in a function can be implicit or explicit; main functions generally don't require them, but are good for cases of software like CLIs. A simple **Hello World**, is like so:

```rs
fn main() {
    print("Hello, World!")
}
```

#### 1.3.2 Project Structure

The project structure in Koboi is simple; assuming the usage of **Pako**, the Koboi project manager, via `pako new <PROJECT_NAME>`, you'd get a file structure, similar to so:

```
ROOT/
  PRJ_NAME/
    kipd.kdf - Koboi Information Package Data, Koboi Datafile
    src/
      main.kb
```

The KIPD file contains information on the compiler, compilation type, flags of use, etc. The KDF file format is a format made for the Koboi programming language, & is interpreted via the compiler.

#### 1.3.3 Building & Running

The build of a project in Koboi varies, via source compilation, project compilation, wrapper compilation, & systems compilation. The building of source compilation is via KoboiC, but can be accessible from **Pako**/**Sendo**, `koboic main.kb -o main`, `pako run main.kb`, sendo, being: `pako build --no-finalize` & `sendo finalize build/app --target windows`.

Sendo is the shipping manager for Koboi, wrapped over Pako, by default, as an easy way to build projects. To run projects, Koboi by default compiles to an executable for you, but to purely run, without compilation, you can use `run`, from Pako, on-top of build, `pako run build`, to emphasize the project, via build, & to say to run it, not build.

#### 1.3.4 Standard Libraries

in dev

### 1.4.0 Basics
#### 1.4.1 Initializing variables

To initialize variables in Koboi, 
