## Main program
[mesin.ino](https://github.com/asaddun/adyawinsa/blob/main/arduino/mesin/mesin.ino)

## 4.0.1 Fix
Bug fix miss IP when sensor reconnecting WIFI

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
