.. _goesrecv:

goesrecv
--------

Demodulate and decode signal into packet stream.

.. important::

   You need a proper antenna system (incl. LNA and filter) to
   receive the LRIT and HRIT signals.

   See :ref:`minimal_receiver` for an example setup.

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

Statistics
==========

There are a few ways to keep an eye on the signal quality and goesrecv
performance. You may use more than one method or none at all.

stdout
^^^^^^

Specify the ``--verbose`` option to make goesrecv periodically write
stats to stdout. This can be useful if you need immediate feedback
about the signal lock and signal quality. The interval can be
controlled with the ``--interval`` option. Also see Options_.

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

You can configure goesrecv to bind the ``stats_publisher`` handlers
for the demodulator and decoder to some valid nanomsg address (see the
`sample configuration`_).

The stats are passed using the nanomsg publish/subscribe
functionality. The stats are sent on a publisher socket. To receive
them you connect a subscriber socket, and subscribe it to the zero
length topic (such that you receive all messages). Refer to
`nn_pubsub(7)`_ for more information about nanomsg publish/subscribe.

.. _`nn_pubsub(7)`: http://nanomsg.org/v1.1.2/nn_pubsub.html

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

You can forward the goesrecv stats to some monitoring and graphing
tool using the statsd_ protocol. This is a simple connection free line
based text protocol that can encode counters, gauges, histograms, etc.
The UDP address to send these stats to is configurable under the
``[monitor]`` section (see the `sample configuration`_).

There are many tools that can deal with the statsd protocol. For
example, tools that folks have used with goesrecv in the past include
InfluxDB_ (self-hosted) and Circonus_ (hosted).

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
