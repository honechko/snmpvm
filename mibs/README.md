# SNMPVM-MIB

Install:

* Place SNMPVM.mib in some directory /path/to/mibs
* ```$ echo 'mibfile /path/to/mibs/SNMPVM.mib' >> ~/.snmp/snmp.conf```  

Usage:

* Show AC 220V/50Hz presence:  
```$ snmpget -v2c -c public 10.0.0.2 power.0```  
```SNMPVM::power.0 = INTEGER: present(1)```  
* Show battery voltage:  
```$ snmpget -v2c -c public 10.0.0.2 voltage.0```  
```SNMPVM::voltage.0 = INTEGER: 12137 millivolts```  
* Show relay state:  
```$ snmpget -v2c -c public 10.0.0.2 relay.0```  
```SNMPVM::relay.0 = INTEGER: deactivated(0)```  
* Activate relay with voltage threshold:  
```$ snmpset -v2c -c private 10.0.0.2 relay.0 i 11000```  
```SNMPVM::relay.0 = INTEGER: 11000```  

