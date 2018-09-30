# Hydra

Hydra (**Hy**perbolic **Dra**wings) is an interpreted programming language that can be used to draw geometric objects in two-dimensional hyperbolic space.

## Installation

Hydra is written in C++ and compiles to a single executable that can be used to interpret Hydra code or work with a REPL.

### Requirements

Compiling Hydra requires [gflags](https://github.com/gflags/gflags) (at least for now) and [glog](https://github.com/google/glog).

### Compilation

Once the required libraries are installed and you checked out the code from this repository, Hydra can be compiled using
```
make release
```

which creates the executable `hydra` in the `bin` directory.

## Usage

Hydra can be used in two ways. First, you can create a file containing Hydra code, e.g. _mycode.hydra_, and interpret it using
```
./bin/hydra mycode.hydra
```

On the other hand, Hydra comes equipped with a REPL mode, which is accessed by calling `./bin/hydra` without passing a file. Once started, you can then enter Hydra code line by line.
