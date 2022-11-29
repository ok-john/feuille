% feuille(1) feuille {VERSION}
% Tom MTT. <tom@heimdall.pm>
% November 2022

# NAME
**feuille** - socket-based pastebin

# SYNOPSYS
**feuille** [-abfhiopstuUvVw]

# DESCRIPTION
**feuille** is a fast, dead-simple socket-based pastebin that allows a
user to send text, logs or code to your server. It focuses on speed,
code quality, and security.

# OPTIONS
**-a address**
: Sets the address that **feuille** will listen on.
: If set to `*`, **feuille** will listen on the IPv6 address `::` and
enable dual-stack mode, which makes **feuille** listen on both IPv4
and IPv6 addresses (won't work on OpenBSD).
: Default: `0.0.0.0`

**-b bytes**
: Sets the buffer size (in bytes) used to receive data from a client.
: A smaller buffer means more memory allocations and exchanges with
  the connection, while a larger buffer induces less memory allocations
  but more loss if not filled completely.
: The difference is minimal, no need to worry about it.
: Default: `131072`B (128KiB)

**-f**
: Makes **feuille** run in the forground.
: Default: disabled

**-h**
: Displays **feuille*'s help page.

**-i length**
: Sets the minimum ID length in characters.
: If a paste with the same ID exists, the length will be increased
(for that paste only).
: Default: `4` (Maximum: `254`)

**-p port**
: Sets the port that **feuille** will listen on.
: Default: `9999`

**-o path**
: Sets the path where **feuille** will output the pastes (and chroot,
if possible).
: Default: `/var/www/feuille`

**-s bytes**
: Sets the maximum size for every paste (in bytes).
: Default: `1048576`B (1MiB)

**-t seconds**
: Sets the timeout for the client to send the paste (in seconds).
: If set to zero, no timeout is set. (Not recommended.)
: Default: `2`s

**-u**
: Sets the user that will be used when dropping root privileges.
: **Warning**: requires root privileges.
: Default: `www`

**-U**
: Sets the base URL which will be prepended to the ID and sent to the
client.
: You do not need to put a slash at the end.
: Default: `http://localhost`

**-v**
: Adds 1 to the verbose level.
: Default: `0`

**-V**
: Displays **feuille**'s version and authors.

**-w**
: Sets the number of processes that will be spawned to handle the
connections.
: Those are *real* processes, not green / posix threads,
you might not want to set this to a huge number.
: Default: the greater of the number of cores in your computer and
`4` workers.

# EXAMPLES

**sudo feuille**
: Runs feuille in the background, chrooting into `/var/www/feuille`,
dropping root privileges and spawning worker processes to accept
incoming connections.

**feuille -p 1337**
: Runs feuille in the background *without* root privileges on port
`1337`.
: **feuille** won't be able to chroot or switch to another user, and
might not be able to write to the default output folder.

**feuille -P ./pastebins/**
: Same as before, but this time with a different path: `./pastebins/`.
: If the folder doesn't exist, it is created with the right
permissions.

**sudo feuille -U "https://bin.heimdall.pm"**
: Runs feuille and sets the base address to `https://bin.heimdall.pm`.

**sudo feuille -w 1**
: Runs feuille "single-threaded".
: (Actually, there's a main thread that does nothing and a thread
that does the actual work.)

**sudo feuille -fvP debug_pastes/**
: Runs feuille in the foreground, with verbose mode enabled, and
makes it output its pastes to the `debug_pastes/` folder.
: Useful for debugging purposes.

**sudo feuille -u nobody**
: Runs feuille using the user `nobody`, instead of user `www`.

**sudo feuille -s 8388608**
: Runs feuille with a maximum file size of 8388608 bytes (8MiB).

**sudo feuille -t 2**
: Runs feuille with a timeout of 2 seconds.

# LOGS
By default, **feuille** runs in the background. The logs should be
located at `/var/log/messages`, if using a standard syslog daemon.
**feuille** doesn't log much, be ready to use the verbose mode for
debugging purposes.

# EXIT VALUES
**0**
: Success

**1**
: Unspecified error

**34**
: Specified number is out of range

**Other**
: Error has been set by a C function

# BUGS
IPs aren't logged. It's not a bug, it's a feature.

Apart from that, none at the moment, as far as I know.

# COPYRIGHT
Copyright © 2022 Tom MTT. <tom@heimdall.pm>
This program is free software, licensed under the 3-Clause BSD License.
See LICENSE for more information.

# APPENDICES
Heavily inspired by [fiche](https://github.com/solusipse/fiche).

I entirely "rewrote" fiche from scratch because I wasn't happy with
some of its features and its overall code quality.
