.. _goesrecv:

goesrecv
--------

Demodulate and decode signal into packet stream.

.. important::

   You need a proper antenna system (incl. LNA and filter) to
   receive the LRIT and HRIT signals.

   **TODO** -- link to guide for building one

You can use goesrecv with an RTL-SDR_ (make sure you have one with the
R820T tuner chip), or an Airspy_ (confirmed to work with the Mini).
The raw signal is then processed by the demodulator and turned into a
stream of 1s and 0s. This is then passed to the decoder where the
bitstream is synchronized and error correction is applied. Every valid
packet is then forwarded to downstream tools (e.g. goeslrit or
goesproc).

.. _rtl-sdr:
  https://rtlsdr.org/
.. _airspy:
  https://airspy.com/

Options
=======

==========================   ==========================================
``-c``, ``--config=PATH``    Path to configuration file
``-v``, ``--verbose``        Periodically show statistics
``-i``, ``--interval=SEC``   Interval for ``--verbose``
==========================   ==========================================

Configuration
=============

The configuration file uses TOML_ syntax. Look further down for a
sample configuration file.

.. _TOML:
   https://github.com/toml-lang/toml

Stats
=====

There are a few ways to keep an eye on the signal quality and goesrecv
performance. You may use more than one method or none at all.

stdout
^^^^^^

See the ``--verbose`` command line option under Options_.

Example output (when running with ``-v -i 1``):

.. code-block:: text

  2018-04-18T04:28:08Z [monitor] gain: 22.54, freq:  2899.3, omega: 1.618, vit(avg):   54, rs(sum):    0, packets: 57, drops:  0
  2018-04-18T04:28:09Z [monitor] gain: 22.51, freq:  2917.1, omega: 1.618, vit(avg):   55, rs(sum):    0, packets: 57, drops:  0
  2018-04-18T04:28:10Z [monitor] gain: 22.53, freq:  2924.1, omega: 1.618, vit(avg):   56, rs(sum):    2, packets: 56, drops:  0
  2018-04-18T04:28:11Z [monitor] gain: 22.53, freq:  2952.2, omega: 1.618, vit(avg):   55, rs(sum):    0, packets: 57, drops:  0

Field glossary:

* ``gain`` -- Multiplier to normalize signal
* ``freq`` -- Frequency offset of signal
* ``omega`` -- Samples per symbol in clock recovery
* ``vit(avg)`` -- Average number of bit errors corrected by Viterbi decoder
* ``rs(sum)`` -- Total number of errors corrected by Reed-Solomon across packets
* ``packets`` -- Number of packets decoded
* ``drops`` -- Number of packets dropped

nanomsg and JSON
^^^^^^^^^^^^^^^^

Configure goesrecv to bind the ``stats_publisher`` handlers for
the demodulator and decoder to something you can connect to (see
`Sample configuration`_).

Example of the raw output of the demodulator stats:

.. code-block:: text

  $ nanocat --sub --connect tcp://127.0.0.1:6001 --raw
  {"timestamp": "2018-04-18T04:52:58.357Z","gain": 2.2494e+01,"frequency": 2.9813e+03,"omega": 1.6181e+00}
  {"timestamp": "2018-04-18T04:52:58.379Z","gain": 2.2494e+01,"frequency": 3.0140e+03,"omega": 1.6181e+00}
  ...

Example of the raw output of the decoder stats:

.. code-block:: text

  $ nanocat --sub --connect tcp://127.0.0.1:6002 --raw
  {"timestamp": "2018-04-18T04:54:22.974Z","skipped_symbols": 0,"viterbi_errors": 44,"reed_solomon_errors": 0,"ok": 1}
  {"timestamp": "2018-04-18T04:54:22.995Z","skipped_symbols": 0,"viterbi_errors": 35,"reed_solomon_errors": 0,"ok": 1}
  ...

statsd
^^^^^^

Passing the statistics to a separate tool for graphing or monitoring
is possible supported through the statsd_ protocol. Statistics are
encoded in a simple line based text protocol and are sent to a
configurable UDP address (see `Sample configuration`_).

Many tools can deal with the statsd protocol. For example, tools that folks have used with goesrecv in the past include InfluxDB_ (self-hosted) and Circonus_ (hosted).

.. _statsd:
  https://github.com/b/statsd_spec
.. _InfluxDB:
  https://www.influxdata.com/time-series-platform/
.. _Circonus:
  https://www.circonus.com/

Sample configuration
====================

.. literalinclude:: ../../etc/goesrecv.conf
  :language: toml

.. toctree::
   :maxdepth: 1
