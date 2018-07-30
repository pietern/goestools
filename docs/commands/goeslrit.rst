.. _goeslrit:

goeslrit
--------

*Assemble LRIT files from packet stream.*

.. note::

  If you're interested only in post-processed data (image and text
  files) you can ignore this command and continue reading about
  goesproc instead.

This tool can be used to turn a stream of packets into LRIT files.
The LRIT file format is unrelated to the LRIT or HRIT *signal* (transmitted by
the GOES-N or GOES-R series, respectively); both these signals use the LRIT file format.

LRIT files can be further processed by goesproc or other tools (such as xrit2pic) to
generate images and text files, or they can be used for debugging purposes.

The tool can either subscribe to a :ref:`goesrecv` process to work with live data,
or it can read packets from files to process recorded data.

Usage
=====

To make ``goeslrit`` write LRIT files to disk, specify either one or more
categories of files you're interested in, or ``--all`` to include all of them.

Run ``goeslrit --help`` to see the full list of options::

  Options:
        --subscribe ADDR  Address of nanomsg publisher
    -n, --dry-run         Don't write files
        --out DIR         Output directory

  Filtering:
        --all             Include everything
        --images          Include image files
        --messages        Include message files
        --text            Include text files
        --dcs             Include DCS files
        --emwin           Include EMWIN files

.. _goeslrit_read_subscription:

Reading packets from goesrecv
=============================

Subscribe to the :ref:`goesrecv` packet publisher to process live data.

This is done using the ``--subscribe`` option, which takes a valid
*nanomsg* address. This is typically a TCP address if goeslrit runs on
a different machine than goesrecv. If they run on the same machine, it
can either be a TCP address or an IPC address.

The address takes the following form:

* :code:`tcp://<ip>:<port>` -- connect to goesrecv over the network.
  Also see `nn_tcp(7) <http://nanomsg.org/v1.1.2/nn_tcp.html>`_.
* :code:`ipc://path/to/socket` -- connect to goesrecv on same machine.
  Also see `nn_ipc(7) <http://nanomsg.org/v1.1.2/nn_ipc.html>`_.

Example::

  $ goeslrit --images --subscribe tcp://1.2.3.4:5005
  Writing: OR_ABI-L2-CMIPM1-M3C02_G16_s20180591958303_e20180591958360_c20180591958427.lrit (4004087 bytes)
  Writing: OR_ABI-L2-CMIPM2-M3C07_G16_s20180592000003_e20180592000073_c20180592000110.lrit (254551 bytes)
  ...

.. _goeslrit_read_files:

Reading packets from files
==========================

The files must be specified in chronological order because they are
read in order. Packets for a single LRIT file can span multiple packet
files, so if they are not specified in chronological order some LRIT
files will be dropped. Specifying a file glob in a Bash shell expands to an
alphabetically sorted list of file names that match the pattern.

Example::

  $ goeslrit --images /path/to/packets/packets-2018-02-28T*
  Reading: /path/to/packets/packets-2018-02-28T00:00:00Z.raw
  Writing: OR_ABI-L2-CMIPM1-M3C02_G16_s20180582358300_e20180582358358_c20180582358429.lrit (4004087 bytes)
  Writing: OR_ABI-L2-CMIPM2-M3C07_G16_s20180590000000_e20180590000071_c20180590000108.lrit (254551 bytes)
  ...
