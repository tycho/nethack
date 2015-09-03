How to rewind a save file that will not load
============================================

If you have a save file that refuses to load, the best way to make it loadable again is to rewind it.  DynaHack save files keep a running log of the game state as you play, so if you know what you're doing, you can get DynaHack to load a previous state in the hope that it is a state that *will* load.

The following document describes how to rewind the save file to a previous state in order to find a state that will load.  These instructions assume that you are using Linux; other operating systems should have equivalent tools to handle this.

Note that DynaHack save files are independent of both operating system and CPU architecture.


# 1. Save file primer

A DynaHack save file has a name like `1441061827_Tariru.nhgame` and can be found in `~/.config/DynaHack/save/` by default.

A save file consists of three parts:

 1. A couple of header lines, the first of which begins with "NHGAME".
 2. A running save log made up of lines of text.
 3. A binary save section that is only present if the player issued the 'save' command to the game.

The techniques that follow will essentially trick DynaHack into thinking the game crashed at an earlier point in its running save log, thereby loading the game at that point instead of the very end (which is presumably unloadable and hence why you're reading this).


# 2. How to rewind a save file

The procedure to rewind a DynaHack save file that will not load is as follows:

 1. Make a backup of the save file.
 2. Truncate to an earlier point in the running save log.
 3. Update the save header so DynaHack will recover to your truncated point.
 4. Backup the truncated save file.
 5. Load the truncated save file.
 6. Play-test the truncated save file.

## 2.1. Backup the save file

This step should speak for itself.  You don't want to mess up this process and not have a backup to fall back on.

## 2.2. Truncate the save file to an earlier point in the running save log

**Open the file in vim**, like so:

    vim -u NONE ~/.config/DynaHack/save/1441061827_Tariru.nhgame

The `-u NONE` ensures that your `.vimrc` is not loaded, which makes loading and editing the save file infinitely faster, otherwise loading will take minutes and editing will be impossibly sluggish.

**Go to the end of the file by pressing `G`.**

**Use Page Up to scroll up past the binary data.**  There may be quite a bit, depending on how deep in the dungeon the character made it.

You'll begin to see lines to text like this, which means you're in the running save log:

    >55e5ac1a:3:0 n k:68 m:[2:] m:[fffffffe:] o:[7:]
    --PFdoYXQgZG8geW91IHdhbnQgdG8gdXNlIG9yIGFwcGx5PyBbLCBjaGlrcXJ4QUVMTU9QVVdZIG9yID8qXTogaD4=
    --PERvIHdoYXQ/OiBQdXQgc29tZXRoaW5nIGludG8gdGhlIGJhZyBjYWxsZWQgaG9sZGluZz4=
    --PFB1dCBpbiB3aGF0IHR5cGUgb2Ygb2JqZWN0cz86IEFsbCB0eXBlcz4=
    --PFB1dCBpbiB3aGF0PzogSD4=
    --WW91IHB1dCBhIGJsZXNzZWQgKzAgcGFpciBvZiBib290cyBjYWxsZWQgd3dhbGsgaW50byB0aGUgYmFnIGNhbGxlZCBob2xkaW5nLg==
    <e112
    ~ f:$435$eNqlkMsNwjAQRFk7duyQcOB3owcCF26YJAiJAwUgcUKijSyFUA490ADiSAUIdvgVALI0sm...
    (etc.)

Now where do you truncate to?  Each of the lines above has a specific meaning, which can help you decide where to truncate from:

 * `>` - A player-issued command e.g. "move", "apply", etc.
 * `--` - A message encoded in base64.
 * `<` - A snippet of RNG state, used as a sort of checksum to detect the validity of the game state after the previous command is replayed if replaying by commands (e.g. using DynaHack's replay feature, or when recovering a crashed game).
 * `~` - A binary save diff, compressed with zlib and base64 encoded.  These are generally the longest lines in the file, and are created at every point where the player could have issued the 'save' command.

Examples in more depth:

    >55e5ac1a:3:0 n k:68 m:[2:] m:[fffffffe:] o:[7:]

`55e5ac1a:3:0 n` is made up of three colon-delimited parts and a command argument at the end: `55e5ac1a` is the hexadecimal timestamp when the command was entered by the player, `3` is the command index plus one ALSO ENCODED IN HEXADECIMAL (you can look up the exact command in the `cmdlist[]` array in `libnitrohack/src/cmd.c`, e.g. 3 == "apply"), `0` is the repeat count, and `n` means the command was not issued with an argument (most manual commands are like this).

What follows are inputs to prompts: `k` is a single key, `m` is a menu selection, `o` represents an object menu selection.

    --PFdoYXQgZG8geW91IHdhbnQgdG8gdXNlIG9yIGFwcGx5PyBbLCBjaGlrcXJ4QUVMTU9QVVdZIG9yID8qXTogaD4=

As before, this is a base64-encoded message.  It can be decoded with standard UNIX tools:

    $ echo 'PFdoYXQgZG8geW91IHdhbnQgdG8gdXNlIG9yIGFwcGx5PyBbLCBjaGlrcXJ4QUVMTU9QVVdZIG9yID8qXTogaD4=' | base64 -d && echo
    <What do you want to use or apply? [, chikqrxAELMOPUWY or ?*]: h>

Now that you know how to read commands and decode logged messages, you need to pick a line to truncate from.  **The last line of the truncated file MUST be a `~` binary save diff line!**

Before choosing the line, note that the very last `~` binary save diff line will reproduce the exact save file state that you have right now, so you should probably skip over and truncate it too.

**Press up and down to position the cursor on the line BELOW the `~` binary save diff line you want to rewind to.**

**To truncate the file** (starting from and including the current cursor line) type the following commands in vim:

 * `V` - Enter linewise visual selection mode.
 * `G` - Scroll to the end of the file.
 * `d` - Delete the selection.

Confirm that the last line of the truncated file is a `~` binary save diff line.

## 2.3. Update the save file header

The very first line of the save file looks something like this:

    NHGAME save 00a65ce4 000030b9 0.5.5

The "save" is a four-letter code indicating the state that DynaHack thinks the save file is in when it last wrote to it.  **Change this to "inpr",** (in progress) so DynaHack will attempt a restoration instead of trying and failing to load the save file by other means.

The first of the two eight-digit hexadecimal numbers is the byte position of the end of the running save log, which needs to be updated.

**Find the length of the truncated file in bytes, convert it to hexadecimal and replace the first hex number in the header with it.**

To find the byte length, press `g` in vim followed by `Ctrl-g`, which will give a line like this at the bottom of the screen:

    Col 1 of 35; Line 1 of 62923; Word 1 of 96586; Byte 1 of 10753190

The very last number is the one you want; you can convert it to hexadecimal in another terminal:

    $ dc
    16 o
    10753190 p
    A414A6

Convert the letter hex digits to lowercase before substituting.

Now save the file by entering `:w`.

The truncated save file is now ready to be loaded in DynaHack, but before that...

## 2.4. Backup the truncated save file

You'll need to play-test the save file in DynaHack, but if you're a server admin you don't want to mess up another player's game by doing this, so back it up first.

## 2.5. Load the truncated save file

Fire up DynaHack and attempt to load the truncated save file.  With any luck, the game's restoration logic will recover the game up to the truncated point in the log.

If not, you'll need to go back and truncate further.

## 2.6. Play-test the truncated save file

If you're up to the point where the game has successfully restored itself, you still need to test that the game is in playable shape.  If the character performed a specific action that lead to the unloadable save, try repeating it.  Kill a few monsters, wait a few turns, change levels, etc.

If the play-test seems to work okay, then you're done!  The save file has been rewinded and play can resume as normal.  If you made a backup of the truncated save file, copy that back in now to undo your play-testing before giving the all-clear.

If there are issues, you'll have to truncate to an earlier point in the running save log.  If the level itself seems corrupted, you'll probably need to rewind up up until prior to the player entering that level.

