Introduction:
Elian APP is an application running on the phone or tablet, it cooperates with the Mediatek IoT WiFi device, 
let the IoT Device which not support display or keypad can connect to the AP without complex operation.

This APP will send target AP router's informations which include SSID/Password... to the air.
The IoT Devices in the smart connection state can listen and decode this information,
Then the IoT Devices can connect to the target AP router with this information.




Release documents:
1. MTK_SmartConnection_Programming_Guide.pdf
2. Elian-*.apk
3. elian_release-*.tgz


Elian-*.apk:
The apk is the application installing on the android wifi enabled platform (Phone/tablet).

elian_release-* tarball:
1:include the library "libelian.a" which can be used in the android /ios app
2:include a android app sample code to start/stop smart connection process by using the "libelian.a"

Programming Guide:
1: How to use the smart connection APIs in the "libelian.a" 
2: Introduce the sample code of android to indicate how to start/stop smart connection on the application
