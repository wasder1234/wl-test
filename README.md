1. Download ESP-IDF
2. Download project files
3. Open sdconfig in ESP-IDF, in ESP-MQTT Configuration menu check Skip publish if disconnected checkbox
4. Connect button to the ESP32 gpio pin #6. Normal-open, close - short to GND.
5. Connect LED to to the ESP32 gpio pin #5. Anode to gpio pin, kathode to GND.
6. Build wl_test project
7. Create account at cloudmqtt.com
8. Create a MQTT server at cloudmqtt.com.

9. In file main.c substitute definition in #define statements after line //--------------- user defined defs -------------- 
	with 
	- your WiFi settings
	- MQTT server parameters you just created - i.e. Server Name, password, port#

10. Download IoT OnOff app to the smartphone
11. In IoT OnOff app add next widgets:
	- widget with bool type, and make subscription to the "homie/wl-test-01/$online" topic. It will show connection status of the wl-test-01 device.
		Set it to display "ready" if value = true, and "lost" if value = false.
	- widget with int type, and make subscription to the "homie/wl-test-01/$stats/signal" topic. It will show signal strength to the device 0 to 100%
	- widget with bool type, and make subscription to the "homie/wl-test-01/input-pin/on-state" topic. It will show state of the button @ pin #6.
		If value = true means button pressed.
	- widget with bool type, and make it to publish into "homie/wl-test-01/led/on-state" topic. This will control LED @ pin #5 from your phone.

12. Flash ESP32, Enjoy.
13. No OTA implemented yet.