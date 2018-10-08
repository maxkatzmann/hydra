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

A detailed explanation of how to use Hydra can be found in the [Getting Started](../../wiki/Getting-Started) section of the wiki.

## Examples

The following Hydra code generates 100 hyperbolic lines where each line starts where the last line ended.
``` swift
// Define the maximum radial coordinate for a point.
var R = 10.0

// Define the variable that holds the starting point of each line.
// The initial value is the origin of the plane.
var point_1 = Pol(r: 0.0, phi: 0.0)

// Iterate 100 times...
for i in [0, 1, 99] {

    // Define the second point of the current line by choosing 
    // radius and angular coordinate uniformly at random.
    var point_2 = Pol(r: random(from: 0.0, to: R), phi: random(from: 0.0, to: 2.0 * M_PI))

    // Draw the line.
    line(from: point_1, to: point_2)

    // The next line starts where the last line ended.
    point_1 = point_2
}

// Save the drawn lines to a file.
save(file: "random_lines.ipe")
```
The result may look something like this
![Drawing of 100 hyperbolic lines.](/examples/random_lines.png)
