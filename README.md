## What is Mandelbrot?

`Mandelbrot` computes a
[Mandelbrot](https://en.wikipedia.org/wiki/Mandelbrot_set) set, parallelized
with, and used to experiment with the features of, the
[Charm++](http://charm.cs.illinois.edu) runtime system to run concurrently,
either on a single multi-core machine or a networked set of computers.

## Build

#### 1. Install prerequisites

- Debian/Ubuntu linux:

   ```
   apt-get install cmake g++ libjpeg-dev libboost1.63-dev
   ```

- Mac OS X:

   ```
   port install cmake clang38 && port select clang mp-clang-3.8
   ```

#### 2. Clone, build third-party libraries, build & test

   ```
   git clone --recursive https://github.com/jbakosi/mandelbrot.git; cd mandelbrot
   mkdir tpl/build; cd tpl/build; cmake ..; make; cd -
   mkdir build; cd build; cmake ../src; make; ./mandel; display out-mandelbrot.jpg
   ```

## Authors

1. [Jozsef Bakosi](https://gitlab.lanl.gov/jbakosi)
