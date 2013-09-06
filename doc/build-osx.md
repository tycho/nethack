Compiling DynaHack on OS X
==========================


1. Install the Homebrew and the Command Line Tools for Xcode
------------------------------------------------------------

 - Homebrew: http://mxcl.github.com/homebrew/
 - Xcode (or the Command Line Tools): http://developer.apple.com/downloads


2. Install dependencies via Homebrew
------------------------------------

    brew install jansson cmake

Ncursesw provided by Homebrew-dupes is currently buggy: https://github.com/Homebrew/homebrew-dupes/issues/43

Until that bug is merged upstream, you'll need to install the patched version:

    brew install https://gist.github.com/gcatlin/3098450/raw/8cc12bc4d095e654876744cdfa6eb57a24186589/ncurses.rb


3. Get the DynaHack source code
-------------------------------

    cd ~
    git clone -b unnethack git://github.com/tung/DynaHack.git dynahack
    cd dynahack


4. Build the game
-----------------

Set the installation location:

    export NH_INSTALL_DIR=$HOME/dynahack
    mkdir build
    cd build
    cmake -DUSE_OSX_HOMEBREW_CURSES=TRUE \
          -DSHELLDIR=$NH_INSTALL_DIR \
          -DBINDIR=$NH_INSTALL_DIR/data \
          -DDATADIR=$NH_INSTALL_DIR/data \
          -DLIBDIR=$NH_INSTALL_DIR/data \
          ..
    make install
