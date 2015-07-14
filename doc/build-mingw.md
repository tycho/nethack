How to compile DynaHack on Windows with MinGW
=============================================

The following steps will build a native Windows version of DynaHack with no dependencies.


1. Install MinGW
-----------------

Download the MinGW installer at (choose the EXE):

    http://sourceforge.net/projects/mingw/files/Installer/mingw-get-inst/mingw-get-inst-20120426/

Run the installer and when asked choose "Download latest repository catalogues".  Stick with the defaults for everything else.

Right click "Computer" on the desktop, choose "Properties", then "Advanced System Settings", then "Environment Variables...".  In the "User variables" box (the top box), click "New..." and enter the name "PATH" and value "C:\MinGW\bin", and press "OK".


2. Install CMake
----------------

Download CMake ("Win32 Installer" under "Binary distributions") at:

    http://cmake.org/cmake/resources/software.html

Run the EXE to install CMake.


3. Compile PDCurses
-------------------

Download pdcurs34.zip at:

    http://sourceforge.net/projects/pdcurses/files/pdcurses/3.4/

Extract it to C:\MinGW\pdcurs34 (make a new folder for it).

Open the command prompt to compile PDCurses with wide character support:

    cd C:\MinGW\pdcurs34\win32
    mingw32-make -f mingwin32.mak WIDE=Y
    rename pdcurses.a libpdcurses.a


4. Compile the game
-------------------

Open the command prompt to install the game's dependencies:

    mingw-get install msys-flex-bin msys-bison-bin mingw32-libz

Click "Download ZIP" at https://github.com/tung/DynaHack and extract it to C:\MinGW.

Open CMake again, this time to configure the build of the game:

 1. Click "Browse Source..." and set it to C:/MinGW/DynaHack-unnethack.
 2. Click "Browse Build..." and set it to C:/MinGW/DynaHack-unnethack/build (make a new folder).
 3. Click "Configure", choose "MinGW Makefiles" and "Use default native compilers".
 4. Set BISON_EXECUTABLE to C:/MinGW/msys/1.0/bin/bison.exe.
 5. Set FLEX_EXECUTABLE to C:/MinGW/msys/1.0/bin/flex.exe.
 6. Set PDCURSES_INC_DIR to C:/MinGW/pdcurs34.
 7. Set PDCURSES_LIB_DIR to C:/MinGW/pdcurs34/win32.

Set the install path for the game to C:/MinGW/DynaHack-unnethack/install (make a new folder) for the following settings:

 * BINDIR
 * CMAKE_INSTALL_PREFIX
 * DATADIR
 * LIBDIR
 * SHELLDIR

Click "Configure", then "Generate" and close CMake.

Open the command prompt to compile the game:

    cd C:\MinGW\DynaHack-unnethack\build
    mingw32-make install

After a few minutes, you can find the game at C:\MinGW\DynaHack-unnethack\install.

Nearly all of the game's options are set and saved in-game, but if you want to customize characters used on the map, see save files or view dump logs of finished games you can find them all in your user's AppData folder under Roaming\DynaHack.

If you want to create a ZIP of the game, you need these files (ignore libnitrohack.dll.a):

 * dynahack.exe
 * nhdat
 * license
 * libnitrohack.dll

You'll also need to include these DLLs, which you can find in C:\MinGW\bin:

 * libgcc_s_dw2-1.dll
 * libz-1.dll
