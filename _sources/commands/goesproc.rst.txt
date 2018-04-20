.. _goesproc:

goesproc
--------

Process stream of packets (VCDUs) or list of LRIT files.

Options
=======

================================   ==========================================
``-c``, ``--config=PATH``          Path to configuration file
``-m``, ``--mode [packet|lrit]``   Process stream of VCDU packets
                                   or pre-assembled LRIT files
``--subscribe ADDR``               Address of nanomsg publisher
``-f``, ``--force``                Overwrite existing output files
================================   ==========================================

If mode is set to ``packet``, goesproc reads VCDU packets from the
specified path(s). To process real time data you can either setup a pipe
from the decoder into goesproc (e.g. use /dev/stdin as path argument),
or use ``--subscribe`` to consume packets directly from :ref:`goesrecv`.
To process recorded data you can specify a list of files that contain
VCDU packets in chronological order.

If mode is set to ``lrit``, goesproc finds all LRIT files in the specified
paths and processes them sequentially. You can specify a mix of files
and directories. Directory arguments expand into the files they
contain that match the glob ``*.lrit*``. The complete list of LRIT files
is sorted according to their time stamp header prior to processing it.

Configuration
=============

The configuration file uses TOML_ syntax. You can
find an example configuration (that includes stanzas for every
available handler) in ``./etc/goesproc.conf``.

.. _TOML:
   https://github.com/toml-lang/toml

Handlers
========

Different products use different handlers with different configuration
options. For example, there is an NWS image handler, NWS text handler,
EMWIN handler, GOES N-series handler, GOES R-series handler, Himawari
handler, etc.

**TODO** -- describe configuration options for every handler

Example
=======

For example, with the following configuration file:

.. code-block:: toml

   [[handler]]
   type = "image"
   product = "goes15"
   region = "fd"
   channels = [ "VS" ]
   crop = [ -2373, 2371, -1357, 1347 ]
   filename = "GOES15_%r_%c_%t"


Running goesproc against a directory with GOES-15 LRIT files
results in a number of image files of the full disk visual channel:

.. code-block:: shell

   $ goesproc --config example.conf --mode lrit ./out/images/goes15/fd
   Writing ./GOES15_FD_VS_20170820-210600.png
   Writing ./GOES15_FD_VS_20170821-000600.png
   Writing ./GOES15_FD_VS_20170821-030600.png
   Writing ./GOES15_FD_VS_20170821-060600.png
   Writing ./GOES15_FD_VS_20170821-090600.png
   Writing ./GOES15_FD_VS_20170821-120600.png
   Writing ./GOES15_FD_VS_20170821-150600.png
   Writing ./GOES15_FD_VS_20170821-180600.png
   ...

You can now use these image files however you like. For example, to
produce a GIF from 8 consecutive full disk images, you can use the
following ImageMagick_ commands:

.. _ImageMagick: https://www.imagemagick.org/

.. code-block:: shell

   $ mogrify -resize '640x480>' *.png
   $ convert -loop 0 -delay 50 *.png GOES15_FD_VS_20170821.gif

And get:

.. image:: /images/GOES15_FD_VS_20170821.gif

Sample configuration
====================

.. literalinclude:: ../../etc/goesproc.conf
  :language: toml

.. toctree::
   :maxdepth: 1
