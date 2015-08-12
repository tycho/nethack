Serving DynaHack via dgamelaunch
================================

This document describes how to setup DynaHack behind a dgamelaunch instance so that people that connect to the dgamelaunch server can play DynaHack.  The following instructions assumes that you already have a working dgamelaunch instance, has only been tested for Linux and assumes you have intermediate knowledge of using Linux.


1. Compile DynaHack
-------------------

Follow the instructions in `doc/build-linux.md`.  The rest of this guide will assume that DynaHack has been built and installed to `~/dynahack/install/`, so the install output so far should look like this:

    ~/dynahack/install/dynahack/dynahack.sh
    ~/dynahack/install/dynahack/dynahack-data/dynahack
    ~/dynahack/install/dynahack/dynahack-data/libnitrohack.so
    ~/dynahack/install/dynahack/dynahack-data/license
    ~/dynahack/install/dynahack/dynahack-data/nhdat


2. Copy relevant DynaHack files into your dgamelaunch chroot
------------------------------------------------------------

Assuming that your dgamelaunch chroot is located at `/opt/dgl-chroot/`:

    $ sudo mkdir /opt/dgl-chroot/dynahack
    $ sudo cp ~/dynahack/install/dynahack/dynahack-data/{dynahack,license,nhdat} /opt/dgl-chroot/dynahack/
    $ sudo cp ~/dynahack/install/dynahack/dynahack-data/libnitrohack.so /opt/dgl-chroot/lib/i386-linux-gnu/

The launch script `dynahack.sh` should not be needed in this case.  Feel free to delete `~/dynahack/install/` if you wish.

The final locations of these files should look like this:

    /opt/dgl-chroot/dynahack/dynahack
    /opt/dgl-chroot/dynahack/license
    /opt/dgl-chroot/dynahack/nhdat
    /opt/dgl-chroot/lib/i386-linux-gnu/libnitrohack.so

Set the correct user and group for the files in `/opt/dgl-chroot/dynahack/`:

    $ sudo chown games:games /opt/dgl-chroot/dynahack/{dynahack,license,nhdat}


3. Copy DynaHack dependencies into your dgamelaunch chroot
----------------------------------------------------------

Use the following commands to work out which libraries you'll need to copy into your dgamelaunch chroot:

    $ ldd /opt/dgl-chroot/dynahack/dynahack
    $ ldd /opt/dgl-chroot/lib/i386-linux-gnu/libnitrohack.so

These libraries should go in `/opt/dgl-chroot/lib/i386-linux-gnu/`, e.g.

 * `libc.so.6`
 * `libdl.so.2`
 * `libncursesw.so.5`
 * `libtinfo.so.5`
 * `libz.so.1`

Some of these libraries may already have been copied in while setting up dgamelaunch itself or setting up other versions of NetHack behind dgamelaunch.


4. Copy locale data for DynaHack into your dgamelaunch chroot
-------------------------------------------------------------

In order for Unicode graphics to work in DynaHack, you must copy locale data into your dgamelaunch chroot, like so:

    $ sudo mkdir /opt/dgl-chroot/usr/lib/locale
    $ sudo cp /usr/lib/locale/locale-archive /opt/dgl-chroot/usr/lib/locale/

This file should end up in `/opt/dgl-chroot/usr/lib/locale/locale-archive`.


5. Configure dgamelaunch to serve DynaHack
------------------------------------------

With all the files needed for DynaHack in place, you will need to edit `/opt/dgl-chroot/etc/dgamelaunch.conf` to allow it to serve DynaHack.  An example game definition:

    DEFINE {
      game_path = "/dynahack/dynahack"
      game_name = "DynaHack 0.5.5"
      short_name = "DYNA"
      game_args = "/dynahack/dynahack", "-H", "/dynahack", "-U", "%ruserdata/%n/dynahack"
      inprogressdir = "%rinprogress-dyna/"
      ttyrecdir = "%ruserdata/%n/ttyrec/dynahack/"
      commands = setenv "LANG" "en_US.UTF-8",
                 setenv "DYNAHACKUSER" "%n"
    }

Notes:

 * The `-H` argument sets the game data directory, where the game finds its `nhdat` and will output `xlogfile`, for example.
 * The `-U` argument sets the user config and save directory, which the game will create if it is not present.
 * The `inprogressdir` *must* exist; make it as so: `sudo mkdir /opt/dgl-chroot/dgldir/inprogress-dyna`.
 * `ttyrecdir` is required if you want ttyrecs for play sessions *and* for spectating.  You may want to create this directory in the `commands[register]` and `commands[login]` dgamelaunch hooks.
 * `setenv "LANG" "en_US.UTF-8"` is required for DynaHack to display Unicode graphics.
 * `setenv "DYNAHACKUSER" "%n"` sets the username that is stored in the "name" field in the `xlogfile` when games end ("charname" stores the name entered for each character).

At the time of writing (August 2015) there is no support for mail (so no spectator messages) or extrainfo (so no dungeon depth/location data in the watch menu) in DynaHack.

Don't forget to add a `play_game "DYNA"` directive in one of your dgamelaunch menu blocks!

With any luck, you should now be able to play DynaHack by connecting to your dgamelaunch server via Telnet or SSH!
