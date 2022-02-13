Resources
=========

Links
-----

About the satellites and the signals (GOES-R series):

* HRIT/EMWIN:

  * https://www.goes-r.gov/users/hrit.html
  * https://www.goes-r.gov/hrit_emwin/ATR-2010-5482.pdf

* GOES Rebroadcast (GRB):

  * https://www.goes-r.gov/users/grb.html
  * https://www.goes-r.gov/users/docs/GRB_downlink.pdf

Because HRIT is similar to LRIT at a higher baud rate, documentation for
LRIT is also relevant for HRIT. The documents below used to be hosted
by NOAA until ~2020. They're mirrored here for posterity.

* :download:`3_LRIT_Receiver-specs.pdf <files/3_LRIT_Receiver-specs.pdf>`
* :download:`4_LRIT_Transmitter-specs.pdf <files/4_LRIT_Transmitter-specs.pdf>`
* :download:`5_LRIT_Mission-data.pdf <files/5_LRIT_Mission-data.pdf>`

The blog series by Lucas Teske is a great resource:

* http://www.teske.net.br/lucas/2016/10/goes-satellite-hunt-part-1-antenna-system/

Notes
-----

Reed-Solomon
^^^^^^^^^^^^

From the LRIT receiver specs:

  The LRIT dissemination service is a Grade-2 service; therefore, the
  transmission of user data will be error controlled using
  Reed-Solomon coding as an outer code. The used Reed-Solomon code is
  (255,223) with an interleaving of I = 4.

Data must be transformed from Berlekamp's dual basis representation to
conventional representation before we can run the Reed-Solomon
algorithm provided by libcorrect.

Refer to `CCSDS 101.0-B-6`_ (Recommendation For Space Data System
Standards: Telemetry Channel Coding) to learn more about the
Reed-Solomon specifics for this application, the dual basis
representation, and how to transform data between conventional
representation and dual basis representation.

.. _`ccsds 101.0-b-6`: https://public.ccsds.org/Pubs/101x0b6s.pdf
