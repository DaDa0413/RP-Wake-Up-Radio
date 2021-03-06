Raspberry Pi Wake-up Radio
==========================
Now is stable version of listen mode
# Purpose

An add-on board attached to the Raspberry Pi's P1 header controls the 5V power
supply in a way that the RasPi is booted upon

* power return after power loss
* a radio message with a pre-defined RFID received by RFM69 transceiver

Approx. 35 seconds after the Raspberry Pi shut down normally the add-on board disconnects
it from its 5V power supply - ready to boot it up again via above triggers.

# Synopsis

## rfwait <I>[localRFID] [GPIO]</I>

`rfwait`: Put the transceiver on RFM69 into DRX mode. 

In that mode the hardware signal goes up upon reception of `localRFID` and thus generates a hardware
wakeup event to RPI.
Even though it called "wait", it won't hang.

`GPIO` is the number of the GPIO in Broadcom SoC nomenclature routed to the
corresponding P1 pin the RFM69's wakeup interrupt hardware signal is connected to.

Example: a value 25 is associated to GPIO25 on SoC which is routed to pin 22 on P1
and Raspberry Pi and thus pin 22 on RPi-GPIO connector on add-on board.

## sudo rfrespond <I>opt.[localRFID] [GPIO]</I>

Upon start of daemon `rfrespond` it checks whether a previously received telegram
is available in the transceiver - e.g. received earlier in DRX mode.
If so, it acknowledges that telegram immediately. Subsequently it waits for
further telegrams matching the `localRFID` and acknowledges it.

Hint: call it by root! (TO write log file in /var/log/radio.log)

## rfwake <I>[calledRFID] opt.[localRFID] [GPIO]</I>

`rfwake` sends periodically telegrams with `localRFID` as SOURCE RFID and
`calledRFID` as DESTINATION RFID until it receives an acknowledge telegram.

Hint: before rfwake is started first the GPIO on Raspberry Pi needs to be configured as interrupt source. This can be done by
```shell
$ gpio edge <GPIO> rising
```

*Simple auto run command on boot*

put the command in /etc/rc.local

http://www.theunixschool.com/2012/06/insert-line-before-or-after-pattern.html

Any `RFID` is a 64 bit value entered as a colon separated sequence of 8 bytes
<B><I>b0:b1:b2:b3:b4:b5:b6:b7</I></B> in hex nomenclature.

## rfwakes (not available now)
wakeup all devices  

using <I>code/waker.py<I> to wakeup device
   
wakeup list format (in code): 
   
   {'locationENG': 'receiver4', 'rfID': '11:11:11:11:11:11:11:14', 'ipAddr': '192.168.10.4', 'lat': 24.786715166666667, 'lng': 120.99349283333333, 'locationCH': 'receiver4'}

# Some Error

1. **Unable to open SPI device: No such file or directory**
   Check does the device exist (ls -l /dev/spi*).
If not enable SPI config:
```shell
$ sudo raspi-config
```
choose **5.Interfacing Options** and enable SPI

2. **Failed to wait for RX(or TX) interrupt**
   Check is GPIO on Raspberry Pi configured as interrupt source.
   If not :
```shell
$ gpio edge <GPIO> rising
```

# Documents

* The add-on board's schematic can be found in
[circuit_schematic.pdf](./Documents/circuit_schematic.pdf).

* The working PCB design diagram is shown in
[PCB_design.ps](./Documents/PCB_design.ps).


* IoT data transfer
[Git link](https://github.com/RayChaung/IoT_Project.git) 
