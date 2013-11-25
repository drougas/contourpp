# Contourpp

Process glucose readings from a Bayer Contour USB meter.

For now, this is a C++ port of [glucodump](https://bitbucket.org/iko/glucodump/), with a few options that I needed to make my life easier.


## Dependencies

* USB interface: Either [hidapi](https://github.com/signal11/hidapi) (default, suggested) or [libhid](http://libhid.alioth.debian.org/).
* Boost date\_time


## Building, Installing

Contourpp has been built and tested successfully on Mac OS X. However, it should be straightforward to build and use it on any platform.

Contourpp is built using [CMake](http://www.cmake.org).

On most systems you can build it using the following commands:

	$ mkdir build && cd build
	$ cmake ..
	$ cmake --build .

### Compiler and linker options

To compile with libhid instead of hidapi, you need to enable the ```CONTOURPP_USE_LIBHID``` option through cmake:

	$ mkdir build && cd build
	$ cmake -DCMAKE_CXX_FLAGS=-DCONTOURPP_USE_LIBHID ..
	$ cmake --build .

### Installation

From the "build" directory run:

	$ make install

… or, just get ```contourpp``` from the ```build/src``` directory.

## Usage

The ```contourpp``` executable reads blood sugar measurements from the Contour USB meter or from a file and prints it to stdout.

* ```contourpp```: Get readings from the Contour USB meter and output them in csv.

* ```contourpp -h```: Print help and exit.

* ```contourpp -l```: Get readings from the Contour USB meter and output them in Bayer's own format. The output of this can be saved on a file and processed later with ```contourpp -f <filename>```.

* ```contourpp -t 04:00 readings.txt```: Get readings from "readings.txt" and correct time by shifting them by 4 hours.

* ```contourpp -a readings.txt```: Filter readings from "readings.txt", printing only the ones with after meal hours.
