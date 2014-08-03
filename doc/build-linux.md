Compiling DynaHack on Linux
===========================

These steps will build a version of DynaHack from source that can be played in Linux.


1. Install dependencies
-----------------------

These are the package names for Ubuntu; other distros may use different names.

 * gcc or clang
 * flex
 * bison
 * libncursesw5-dev
 * libjansson-dev
 * zlib1g-dev
 * cmake
 * cmake-curses-gui


2. Configure the build
----------------------

After getting and extracting the game's source code (e.g. to ~/dynahack), create a build directory and run CMake:

    cd ~/dynahack
    mkdir build
    cd ~/dynahack/build
    cmake ..

Use the CMake GUI to set the install path:

    cd ~/dynahack/build
    ccmake .

Set SHELLDIR and CMAKE_INSTALL_PREFIX to /home/username/dynahack/install.

Set BINDIR, DATADIR and LIBDIR to /home/username/dynahack/install/dynahack-data.

Press 'c' to configure, then 'g' to generate the build files and exit.


3. Compile the game
-------------------

Enter the following commands to compile the game:

    cd ~/dynahack/build
    make install

To play the game, run its launch script:

    ~/dynahack/install/dynahack.sh

Most options can be set and saved in-game, but if you want to customize characters used on the map, see save files or view dump logs of finished games you can find them all in ~/.config/DynaHack.

Programs on Linux use baked-in paths at compile time, so it's best to compile the game from source rather than try to get a pre-compiled version to work.
