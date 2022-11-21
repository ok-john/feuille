# feuille

**feuille** is a fast, dead-simple socket-based pastebin that allows a
user to send text, logs or code to your server. It focuses on speed,
code quality, and security.

<details>
    <summary>Table of Contents</summary>
    <ol>
        <li><a href="#usage">Usage</a></li>
        <li>
            <a href="#installation">Installation</a>
            <ul>
                <li><a href="#dependencies">Dependencies</a></li>
                <li><a href="#build">Build</a></li>
                <li><a href="#configuration">Configuration</a></li>
            </ul>
        </li>
        <li>
            <a href="#help">Help</a>
            <ul>
                <li><a href="#how-do-i-make-feuille-run-at-startup">How do I make feuille run at startup?</a></li>
                <li><a href="#how-do-i-remove-expired-pastes-after-some-time">How do I remove expired pastes after some time?</a></li>
                <li><a href="#how-can-i-send-pastes-directly-from-my-website-instead-of-using-netcat">How can I send pastes directly from my website (instead of using netcat)?</a></li>
            </ul>
        </li>
        <li><a href="#authors">Authors</a></li>
        <li><a href="#license">License</a></li>
        <li><a href="#acknowledgments">Acknowledgments</a></li>
    </ol>
</details>

## Usage

You'll need either `nc` or nmap's `ncat`.

The former is probably already available on your system, but lacks
some features, which will make you wait for the timeout before
getting a link to the paste (usually 4 seconds).

The latter will have to be installed manually but is more featureful
and will get a link instantly once everything is sent. It works the
exact same way as `nc`.

*Choose your weapon wisely.*

Assuming you're using my personal pastebin instance,
[heimdall.pm](https://bin.heimdall.pm/), you can upload text, code or
logs like this:

```console
// sending text
$ echo Hello, World! | nc heimdall.pm 8888
https://bin.heimdall.pm/abcd

// sending files
$ cat feuille.c | nc heimdall.pm 8888
https://bin.heimdall.pm/efgh
```

*This truly is the joy of Unix pipes.*

Once you received the link to your paste, you can send it to someone,
browse it or `curl` it, like this:

```console
$ curl https://bin.heimdall.pm/abcd
Hello, World!

$ curl https://bin.heimdall.pm/efgh
/*
 * feuille.c
 *  Main source file.
...
```

Want to push the concept further? You can send encrypted files, too!
You'll need `gpg` for this. `-c` means encryption using a password,
`-ao tmp.pgp` means ASCII output to file `tmp.pgp`.

```console
$ cat secret.txt | gpg -cao tmp.pgp
$ cat tmp.pgp | nc heimdall.pm 8888
https://bin.heimdall.pm/ijkl
```

You can then retrieve it and decrypt it using `curl` and `gpg` again,
like this:

```console
$ curl https://bin.heimdall.pm/ijkl | gpg -d
```

(Obviously, you'll have to type the right password.)

But, all those commands are really cumbersome, aren't they?  
Guess what? We made aliases!

Put those into your `~/.{ba,z,k}shrc`:

```sh
alias pst="$NC heimdall.pm 8888"
alias spst="gpg -cao tmp.pgp && cat tmp.pgp | $NC heimdall.pm 8888 && rm tmp.pgp"
```

Now, you can use **feuille** like this:

```console
// plain
$ echo Hello, World! | pst
https://bin.heimdall.pm/mnop

// encrypted
$ echo da sup3r sekr1t | spst
https://bin.heimdall.pm/qrst
```

For a complete list of aliases, see
[here](https://basedwa.re/tmtt/feuille/src/branch/main/misc/aliases)
and either put those in your `~/.{ba,z,k}shrc` or source it.

That sould be it. Have fun!

## Description

* Focuses on speed,
    * Multi-threaded (using `fork`)
    * Only does what it needs to do

* code quality,
    * Readable, documented code
    * With future contributors / maintainers in mind

* and security
    * `chroot`s in the output folder
    * Drops root privileges once they're no longer needed
    * Uses OS-specific security measures (like OpenBSD's `pledge`)

* Plenty of auxiliary files (see
[misc/](https://basedwa.re/tmtt/feuille/src/branch/main/misc),
[cgi/](https://basedwa.re/tmtt/feuille/src/branch/main/cgi) and
[cron/](https://basedwa.re/tmtt/feuille/src/branch/main/cron))
    * A list of aliases for your users' `~/.{ba,z,k}shrc`
    * A CGI script that lets the user send pastes directly from your
      website
    * A sample HTML form for your CGI script
    * A cron job that deletes expired pastes
    * A cron job that runs **feuille** at startup

* Lots of options (see [configuration](#configuration))
* Works on nearly all POSIX-compliant OSes
* Can be run in the background and as a service
* IPv6-enabled

## Installation

### Dependencies

You'll need a working *POSIX-compliant system* such as Linux, OpenBSD
or FreeBSD, a C99 compiler (GCC, Clang...) and a POSIX-Make
implementation.

You'll probably want an HTTP / Gopher / Gemini / ... server to serve
the pastes on the web, such as OpenBSD's httpd or Apache. Just make
your server serve the folder feuille's using, there are plenty of
tutorials on the web.

If you wish to make modifications to the manpage, you'll need pandoc
to convert the markdown file into a man-compatible format.

### Build

**feuille** needs to be built from source.

To do so, you'll first need to clone the repository.

```console
$ git clone https://basedwa.re/tmtt/feuille
$ cd feuille
```

Then, simply run:

```console
$ make
$ sudo make install
```

If you wish to make a debug build, you can set `DEBUG` to whatever
comes to your mind.

You can also set `CC` to the compiler of your liking, like `clang` or
`pcc`.

```console
$ make DEBUG=yes CC=clang
```

In order to compile CGI script(s), run:

```console
$ make cgi
```

`ADDR` and `PORT` can be set to the address and port on which
**feuille** listens, respectively.

### Configuration

For a complete list of options and examples, please see the manpage,
either on your computer by doing `man feuille` or on the
[online wiki](https://basedwa.re/tmtt/feuille/wiki/Manpage).

## Help

* [How do I make feuille run at startup?](#how-do-i-make-feuille-run-at-startup)
* [How do I remove expired pastes after some time?](#how-do-i-remove-expired-pastes-after-some-time)
* [How can I send pastes directly from my website (instead of using netcat)?](#how-can-i-send-pastes-directly-from-my-website-instead-of-using-netcat)

### How do I make feuille run at startup?

You can put that in your crontab (by doing `sudo crontab -e`).
It will start **feuille** every time the system starts.
No need for some fancy service file :DDD

Obviously, you can put some options after `feuille`, like `-o
/var/feuille` or `-v`.

```
@reboot feuille
```

See
[cron/startup.cron](https://basedwa.re/tmtt/feuille/src/branch/main/cron/startup.cron)
if you'd like to download the cron job.

### How do I remove expired pastes after some time?

You can put that in your crontab (by doing `sudo crontab -e`).
It will delete all files in `/var/www/feuille` that are at least 7
days old.

Don't forget to change the folder to the one **feuille**'s using and
eventually `+7` to the maximum file age you'd like to use.

```
0 0 * * * find /var/www/feuille -type f -mtime +7 -exec rm {} +
```

See
[cron/purge.cron](https://basedwa.re/tmtt/feuille/src/branch/main/cron/purge.cron)
if you'd like to download the cron job.

### How can I send pastes directly from my website (instead of using netcat)?

We made a CGI script for that. First, you need to build it:

```console
$ make cgi
```

`ADDR` and `PORT` can be set to the address and port on which
**feuille** listens, respectively.

Once it's done, you can put `./web/cgi/feuille.cgi` in your website's
`cgi-bin` folder (usually somewhere like
`/var/www/my.paste.bin/cgi-bin`) and configure your web server
to execute CGI scripts.

You can then create an HTML form that will send a POST request to the
CGI script. The form must have `enctype="text/plain"` and must contain
only one input or textarea. See
[web/cgi/form.html](https://basedwa.re/tmtt/feuille/src/branch/main/cgi/form.html)
for a sample form.

## Authors

Tom MTT. <tom@heimdall.pm>

## License

Copyright © 2022 Tom MTT. <tom@heimdall.pm>
This program is free software, licensed under the 3-Clause BSD License.
See LICENSE for more information.

## Acknowledgments

Heavily inspired by [fiche](https://github.com/solusipse/fiche).

I entirely "rewrote" fiche from scratch because I wasn't happy with
some of its features and its overall code quality.
