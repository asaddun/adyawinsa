WemosD1R2_v1.2

Wemos Report

1. Websocket Client ✅
2. WiFi Manager ✅
3. Network Protocol Time ✅
4. Over the Air ✅ (tapi terblokir dari jaringan kantor)
5. Tiap board bisa menampilkan hanya cycle data masing-masing (dari machine id mereka)

Masalah:
(Opini sendiri) dengan sambungan API-HOTSPOT
1. Bila board terhubung dengan IP 192.168.1.xxx, board ga bisa nyambung ke websocket server (laptop),
jadi saat IP board 192.168.10.xxx board bisa nyambung ke websocket server.
2. Bila IP laptop 192.168.1.xxx dan board juga 192.168.1.xxx, maka tidak bisa melakukan OTA (over the air) dengan board, jika kedua IP selain 192.168.1.xxx maka bisa melakukan OTA.
3. IP board 192.168.1.xxx maka tidak bisa buka webpage milik board (Webserver), IP 192.168.10.xxx bisa buka webpage
milik board (Webserver).

Kemarin coba pake AP sendiri (tidak terhubung internet), OTA bisa dilakukan, Websocket bisa terhubung tapi langsung terputus
karena tidak ada internet. 