## Wiring

![wiring](https://i.imgur.com/cKlN3Vx.jpg)  

D6 as a Clamp Signal  
D7 as an Inject Signal  
5V from Arduino connect to COM Relay  
NO Relay connect to D6/D7  
D6 and D7 with Resistor 1k connect to GND of Arduino

## Flow
![wiring](https://i.imgur.com/h5RQgyR.png)  

## Release Change Log:

## 4.0.3 Update
- Add the ability to store the temporary data to the memory using FS file when the connection of sensor is problem, then send data again when connection is reestablised as a JSON Array format.  
- Change the way send the time, now using the **Epoch time** (second from 01-01-1970), in this code the epoch is in GMT 00:00, using JS funtion `toLocaleTimeString()` or `toLocaleDateString()` will convert the epoch to the time zone area of the system.

## 4.0.2 Fix
Reconnect saved WIFI if after several time sensor become AP and not configured.

## 4.0.1 Fix
Bug fix miss IP when sensor reconnecting WIFI.

## 4.0.0 Release
#### Always reconnect WIFI
    Ability to reconnect to Wifi if disconnected, without entering Wifi Manager Portal.
    (in some long time, if still disconnected, it will open the Wifi Manager Portal)

#### 'Reset' button at dashboard
    Ability to reset WIFI Credential to make changes in Wifi Manager Portal.

#### Over the Air
    Ability to upload code without connect the data cable, Limited by IP Segment.

#### Network Protocol Time
    Request Network Time to be send as data.

#### WiFi Manager
    Adding Wifi Manager to easy connect to WIFI and set parameter of machine ID in Wifi Manager Portal.

#### Websocket Client & Server
    Adding Websocket to communicate the data to server and dashboard.
