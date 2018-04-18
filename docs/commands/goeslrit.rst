.. _goeslrit:

goeslrit
========

.. note::

  If you're only interested in post-processed data (image and text
  files) you can ignore this command and continue reading about
  goesproc instead.

This tool can be used to turn a stream of packets into LRIT files.
LRIT files can then be used by goesproc (or other tools, such as to
generate usable images and text files, or for debugging purposes.

It can either read packets from files to process recorded data, or
subscribe to :ref:`goesrecv` process to work with live data.

To make it write LRIT files to disk, you have to specify the category
of files you are interested in. Pass ``--help`` for a list of
filtering options. To make it write **ALL** LRIT files it seems, run
goeslrit with the ``--all`` option.

Reading packets from files
--------------------------

The files must be specified in chronological order because they are
read in order. Packets for a single LRIT file can span multiple packet
files, so if they are not specified in chronological order some LRIT
files will be dropped. Specifying a file glob in Bash expands to an
alphabetically sorted list of file names that match the pattern.

Example::

  $ goeslrit --images /path/to/packets/packets-2018-02-28T*
  Reading: /path/to/packets/packets-2018-02-28T00:00:00Z.raw
  Writing: OR_ABI-L2-CMIPM1-M3C02_G16_s20180582358300_e20180582358358_c20180582358429.lrit (4004087 bytes)
  Writing: OR_ABI-L2-CMIPM2-M3C07_G16_s20180590000000_e20180590000071_c20180590000108.lrit (254551 bytes)
  ...

Reading packets from goesrecv
-----------------------------

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
