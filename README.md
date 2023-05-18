# SNMP voltmeter

![SNMP voltmeter](https://github.com/honechko/snmpvm/raw/main/snmpvm.jpg)
_(Prototype without relay)_

Device is used for SNMP monitoring of battery backup systems. Device has one
input for battery voltage measurement, the same input is used for device's
power supply. Device also has one input for detection of AC 220V/50Hz presence
and one relay with NC contacts (optionally with NO & NC contacts). Relay can
be activated or deactivated by SNMP commands, device can automatically
deactivate relay after timeout or when battery voltage drops below threshold.
Last feature can be used for safe testing of battery lifetime and obtaining
discharge curve. If possible, device can be placed inside battery charger,
making it "smart".

![Usage](https://github.com/honechko/snmpvm/raw/main/docs/usage.png)

Examples:

* Show AC 220V/50Hz presence:  
```$ snmpget -On -v2c -c public 192.168.0.2 .1.3.6.1.4.1.9.2.1.7.0```  
* Show battery voltage:  
```$ snmpget -On -v2c -c public 192.168.0.2 .1.3.6.1.4.1.9.2.1.8.0```  
* Show relay state:  
```$ snmpget -On -v2c -c public 192.168.0.2 .1.3.6.1.4.1.9.2.1.9.0```  
* Activate relay with voltage threshold:  
```$ snmpset -On -v2c -c private 192.168.0.2 .1.3.6.1.4.1.9.2.1.9.0 i 11000```  
* Activate relay for 5 seconds:  
```$ snmpset -On -v2c -c private 192.168.0.2 .1.3.6.1.4.1.9.2.1.9.0 i -5```  
* Deactivate relay:  
```$ snmpset -On -v2c -c private 192.168.0.2 .1.3.6.1.4.1.9.2.1.9.0 i 0```  

## Network setup

On power-up device waits 15 seconds for setup packet then loads latest
configuration and switches to handling SNMP requests. If setup packet is
received then new configuration from it is used. Setup packet is a raw
ethernet frame, for setup procedure you must connect device directly to
Linux or FreeBSD host using cross-over cable (device does not support MDIX),
no switches must be used in between. Use
[util/snmpvmsetup.c](https://github.com/honechko/snmpvm/raw/main/util/snmpvmsetup.c)
to send setup packet.

Examples:

* Command to compile snmpvmsetup:  
```$ cc snmpvmsetup.c -o snmpvmsetup```  
* Show help message:  
```# ./snmpvmsetup```  
* Change network configuration only:  
```# ./snmpvmsetup eth0 48:4f:4e:45:59:5f 10.0.0.2 255.255.255.0 10.0.0.1```  
* Change network configuration and voltage multiplier (see below):  
```# ./snmpvmsetup eth0 48:4f:4e:45:59:5f 10.0.0.2 255.255.255.0 10.0.0.1 31915```  

Notes: Command requires root privileges. MAC-address is also configurable,
you must provide unique. There is no way to set voltage multiplier without
modifying network settings. Skipping voltage multiplier parameter or setting
it to 0 keeps previous setting unchanged on device. There is no way to fetch
current settings from device. It's safe to send few setup packets to ensure
speed/duplex negotiation has passed and device received at least one.

## Calibration

Battery voltage is calculated as a value from analog-to-digital converter
(ADC) multiplied by "voltage multiplier". In order to match measured and
real battery voltages, appropriate voltage multiplier must be chosen.

Use this algorithm to calculate voltage multiplier:

1. Power-cycle device and reset voltage multiplier to known value:  
```# ./snmpvmsetup eth0 48:4f:4e:45:59:5f 10.0.0.2 255.255.255.0 10.0.0.1 32768```  
2. Measure battery voltage by device:  
```$ snmpget -On -v2c -c public 10.0.0.2 .1.3.6.1.4.1.9.2.1.8.0```  
For example you have obtained:  
```.1.3.6.1.4.1.9.2.1.8.0 = INTEGER: 12413```  
3. Measure the same battery voltage by other method. For example with
multimeter you have obtained value: 12.09V
4. Calculate Vmul: ADC * 32768 = 12413; ADC * Vmul = 12090; solving equations:
Vmul = 32768 * 12090 / 12413 = ~31915
5. Power-cycle device and set voltage multiplier:  
```# ./snmpvmsetup eth0 48:4f:4e:45:59:5f 10.0.0.2 255.255.255.0 10.0.0.1 31915```  

## Tips and tricks

* Command to calculate the average value over 10 measurements:  
```$ awk 'BEGIN{c="snmpget -On -v2c -c public 10.0.0.2 .1.3.6.1.4.1.9.2.1.8.0";for(s=i=0;i<10;i++){c|getline;close(c);s+=$4}print s/10}'```  
* There is no advantage of 100Mbps over 10Mbps for this device, moreover at
10Mbps device consumes less power from battery and speed/duplex negotiation
takes less time. In Linux and FreeBSD use command respectively:  
```# mii-tool eth0 -A 10baseT-FD```  
```# ifconfig em0 media 10baseTX mediaopt full-duplex```  

