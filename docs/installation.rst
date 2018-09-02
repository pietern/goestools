.. _installation:

Installation
============

Building goestools requires Linux.

It can be built for x86 and ARM (with NEON).

.. note::

  Other operating systems may be fine as well but have not been
  confirmed to work. If you manage to get goestools to work on macOS,
  Windows, or something else, please reach out and share instructions,
  so they can be added to this page.

Dependencies
------------

System dependencies:

* CMake
* C++14 compiler
* OpenCV (for image processing in goesproc)
* zlib (for decompressing EMWIN data)

Bundled dependencies (see vendor directory in repository):

* libcorrect (currently a fork with CMake related fixes)
* libaec
* nanomsg
* json
* tinytoml

Building
--------

These instructions should work for both Ubuntu and Raspbian.

Install system dependencies:

.. code-block:: text

  sudo apt-get install -y \
    build-essential \
    cmake \
    git-core \
    libopencv-dev \
    libproj-dev \
    zlib1g-dev

If you want to run goesrecv on this machine, you also have to install
the development packages of the drivers the SDRs you want to use;
``librtlsdr-dev`` for an RTL-SDR, ``libairspy-dev`` for an Airspy.

Now you can build and install goestools:

.. code-block:: text

  git clone --recursive https://github.com/pietern/goestools
  cd goestools
  mkdir build
  cd build
  cmake .. -DCMAKE_INSTALL_PREFIX=/usr/local
  make
  make install

The goestools executables are now available in /usr/local/bin.
