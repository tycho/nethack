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
 * zlib1g-dev
 * cmake


2. Configure the build
----------------------

After getting and extracting the game's source code (e.g. to ~/dynahack), create a build directory and run CMake:

    cd ~/dynahack
    mkdir build
    cd ~/dynahack/build
    cmake .. -DINSTALL_BASE=~/dynahack/install


3. Compile the game
-------------------

Enter the following commands to compile the game:

    cd ~/dynahack/build
    make install

To play the game, run its launch script:

    ~/dynahack/install/dynahack/dynahack.sh

Most options can be set and saved in-game, but if you want to customize characters used on the map, see save files or view dump logs of finished games you can find them all in ~/.config/DynaHack.
