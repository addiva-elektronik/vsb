Virtual Serial Bus
==================

Very silly program that creates Linux PTYs that act like a serial bus.

> **Note:** you can probably achieve the same thing with [socat(1)][].


Usage
-----

The smallest setup is `vsb -n 1`, but that's pretty useless since you
only allocate a bus with a connection for a single serial device.  A
better example is something like this:

    $ vsb -n 2
	vsb: device 1 => /dev/pts/3
	vsb: device 2 => /dev/pts/4

Pay attention to the device node names, they will differ depending on
your system.  A future version of this program may add support for
launching programs on these devices, or other scripting possibilities.

Here we got `/dev/pts/3` and `/dev/pts/4` which are PTY slave devices
where their respective master devices are bridged by `vsb`.


Origin & References
-------------------

Made by Addiva Elektronik AB, Sweden.  Available as Open Source under
the MIT license.  Please note, libus has a 3-clause BSD license which
contains the advertising clause.

[socat(1)]: https://manpages.org/socat
