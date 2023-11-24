.. _launch_mikroe:

Arduino UNO click shield
########################

Overview
********

This is a config for Launch's custom MIKROE click shield.

The first socket (``mikrobus_header_1``) is the default socket which is
assigned the node label ``mikrobus_header`` in the overlay.


Requirements
************

This shield can only be used with a board which provides a configuration
for Arduino R3 connector.

The board must also define node aliases for arduino Serial,
SPI and I2C interfaces (see :ref:`shields` for more details).

Connecting shields should use the first socket (``mikrobus_header_1``). This
socket is assigned the ``mikrobus_header`` node label.

Programming
***********

Include ``-DSHIELD=launch_mikroe`` when you invoke ``west build`` with
other mikroBUS shields. For example:

.. zephyr-app-commands::
   :zephyr-app: samples/net/sockets/echo_server
   :host-os: unix
   :board: sam_v71_xult
   :gen-args: -DOVERLAY_CONFIG=overlay-802154.conf
   :shield: "arduino_uno_click atmel_rf2xx_mikrobus"
   :goals: build

References
**********

.. target-notes::

.. _Arduino UNO click shield website:
   https://www.mikroe.com/arduino-uno-click-shield
