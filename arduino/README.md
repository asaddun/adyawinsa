WemosD1R2_v1.2

Wemos Report

1. Websocket Client ✅
2. WiFi Manager ✅
3. Network Protocol Time ✅
4. Over the Air ✅ (tapi terblokir dari jaringan kantor)
5. Tiap board bisa menampilkan hanya cycle data masing-masing (dari machine id mereka)

Masalah:
(Opini sendiri) dengan sambungan API-HOTSPOT
1. Bila board terhubung dengan IP xxx.xxx.1.xxx, board ga bisa nyambung ke websocket server (laptop),
jadi saat IP board xxx.xxx.10.xxx board bisa nyambung ke websocket server.
2. Bila IP laptop xxx.xxx.1.xxx dan board juga xxx.xxx.1.xxx, maka tidak bisa melakukan OTA (over the air) dengan board.

Kemarin coba pake AP sendiri (tidak terhubung internet), OTA bisa dilakukan, Websocket bisa terhubung tapi langsung terputus
karena tidak ada internet. 