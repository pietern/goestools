.. _minimal_receiver:

A minimal LRIT/HRIT receiver
============================

Receiving the LRIT and/or HRIT signal can be done with relatively
inexpensive equipment. This guide describes a minimal configuration
that I have confirmed to work **at my location**.

.. warning::

   Whether or not this configuration works at your location depends on
   a large number of factors, such as satellite elevation, local
   interference, etc. Try it at your own risk.

The bill of materials is as follows:

* Raspberry Pi 2 (v1.1+) or higher

  * Available at Newark__; $35 (Raspberry Pi 3 Model B+)

* RTL-SDR with R820T2 tuner

  * Available at NooElec__; $24 (NooElec NESDR SMArt)

* NooElec SAWBird (or SAWBird+) with bias tee (LNA and filter board)

  * Available at NooElec__; ~$25 (or $35 for the SAWBird+)

* 1.9 GHz Parabolic Grid Antenna

  * Available at Excel-Wireless__ (or Amazon); ~$100

* Adapter from male Type N to male SMA (for antenna to LNA)

  * Available at Amazon__ and elsewhere; $7

* Jumper from male SMA to male SMA (for LNA to RTL-SDR)

  * Available at NooElec__; $8

This sums up to about $200 excluding tax/shipping.

.. __: http://www.newark.com/raspberry-pi/2773729/sbc-arm-cortex-a53-1gb-sdram/dp/49AC7637
.. __: http://www.nooelec.com/store/sdr/sdr-receivers/nesdr/nesdr-smart-sdr.html
.. __: http://www.nooelec.com/store/sawbird-goes.html
.. __: https://www.excel-wireless.com/1900-mhz-grid-parabolic-antenna/1850-1990-mhz-grid-parabolic-antenna-20-dbi
.. __: https://www.amazon.com/gp/product/B01MFHRW4N/
.. __: http://www.nooelec.com/store/sdr/sdr-adapters-and-cables/sdr-cables/male-sma-to-male-sma-pigtail-rg316-0-5-length.html

Instead of the listed RTL-SDR, you can also opt to buy the `NESDR
SMArTee`__ ($26) which has an always-on bias tee for powering the
SAWBird SAWBird LNA. Without an integrated bias tee you'll have to
supply power to the SAWBird yourself with a micro-USB cable (see
pictures below).

Another option is to buy the `NESDR SMArTee XTR`__ ($38) which is the same
as the above SDR, but has an extended frequence range that may be better able
to tune into the 1694 Mhz signal (which is fairly close to the 1750 Mhz limit
in some SDRs)

.. __: http://www.nooelec.com/store/sdr/sdr-receivers/nesdr/nesdr-smartee-sdr.html
.. __: http://www.nooelec.com/store/sdr/sdr-receivers/nesdr/nesdr-smartee-xtr-sdr.html

.. note::

   Additional items such as power supplies or mounting hardware
   are not listed here.

.. important::

   A similar grid dish antenna works for me at my location (San
   Francisco Bay Area; the exact antenna is no longer available).
   I can receive GOES-15 at its west location
   (135 degrees west), GOES-16 at its east location (75 degrees west),
   and GOES-17 at its checkout location (89 degrees west). GOES-16 is
   furthest away and at 25 degrees elevation at my location.

   If your location has the satellite you're interested in at a lower
   elevation than 25 degrees, you may need a bigger dish. However,
   even with any satellite at a higher elevation, there are other
   factors that can impact whether or not you can use the dish
   mentioned here (blocked line of sight, local interference, etc).

   Find your local azimuth and elevation on `Satview`_
   (`GOES-15 <http://www.satview.org/?sat_id=36411U>`_,
   `GOES-16 <http://www.satview.org/?sat_id=41866U>`_,
   `GOES-17 <http://www.satview.org/?sat_id=43226U>`_).

.. _satview: http://www.satview.org/

Hardware
--------

The setup to test this configuration looked like this.

.. image:: minimal_zoomed.jpg
   :scale: 45 %
.. image:: minimal_overview.jpg
   :scale: 45 %

* The grid dish antenna has a female Type N connector. To connect it
  to the SAWBird you need a male Type N to male SMA adapter (in the
  picture you see the adapter and an male SMA to male SMA jumper).
* The NooElec SAWBird is connected to the grid dish antenna on the
  input and the RTL-SDR on the output. It is powered by the Pi over
  USB (the bias tee version of the SAWBird has a micro-USB connector).
* The RTL-SDR is connected to the SAWBird using a male SMA to male SMA
  jumper cable.
* In this setup, the Pi gets its power over USB.
* In this setup, the Pi is connected to the network over Ethernet.
* The laptop in the right image was only used to SSH into the Raspberry
  Pi.

.. note::

   While the Raspberry Pi 2 v1.1 is adequate for HRIT demodulation and
   decoding, it doesn't leave much margin on the processing power. In
   a screenshot below you can see the demodulator use 96% of a single
   CPU core.

   The RTL-SDR can be configured to use a low enough sample rate to
   still work with the Pi 2. If you want to use an Airspy Mini instead
   (which has a minimum sample rate of 3M samples/sec), you'll have to
   use a Raspberry Pi 2 v1.2 or the Raspberry Pi 3.

Software
--------

Raspbian works fine. There is no need for a desktop environment if you
only use the Pi for signal demodulation and decode, so you can use the
lite version. For instructions on building and installing goestools,
see :ref:`installation`.

After installing :ref:`goesrecv`, copying and modifying its
configuration file, you can run it with ``-v -i 1`` to get
second-by-second demodulator statistics.
You can use the Viterbi error rate for pointing your dish.
A rate of ~2000 means there is no signal.
A rate of ~1000 may give you sporadic packets.
A rate of ~400 can give you the complete stream of packets without any
drops, but is very sensitive to interference.
A rate of ~100 or lower is good and can give you a packet stream
without drops for hours on end.
The lower the Viterbi error rate, the better your signal.

The output of :ref:`goesrecv` during operation of the test setup:

.. image:: minimal_goesrecv.png

The output of ``htop`` during signal lock:

.. image:: minimal_htop.png

To process the packet stream, see :ref:`goeslrit` and :ref:`goesproc`.

For example, this is the false color full disk received from GOES-16
and assembled by goesproc during this test (resized to 1024x1024 for
size constraints):

.. image:: minimal_GOES16_FD_FC_20180505T223038Z_full.jpg

This is a crop of Northern America to get an impression of the
resolution of these full disk images:

.. image:: minimal_GOES16_FD_FC_20180505T223038Z_crop.jpg

Notes
-----

* For enclosures, check out `Bud Industries
  <https://www.budind.com/>`_ and `Hammond Manufacturing
  <https://www.hammfg.com/enclosures>`_.
* The power consumption of this setup is about 6 watts.
* Other ARM based single board computers should work fine as well as
  long as they have comparable (or better) performance to the
  Raspberry Pi 2.
* Empirical evidence shows that adding another LNA *after* the NooElec
  SAWbird improves signal quality (e.g. going from Viterbi error rate
  ~150 to ~100).
* `This post`__ by `@usa_satcom <https://twitter.com/usa_satcom>`_
  showing the grid antenna is capable of receiving LRIT.

.. __: https://twitter.com/usa_satcom/status/820773345956200449
