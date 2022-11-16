WemosD1R2_v1.1

Wemos Report

1. Websocket Client
2. WiFi Manager (custom parameter di bagian setup)
3. Network Protocol Time
4. Over the Air (tapi terblokir dari jaringan kantor)
5. Tiap board bisa menampilkan hanya cycle data masing-masing
6. Tombol 'Reset' di webpage masing-masing board

Masalah:
(Opini sendiri) dengan sambungan API-HOTSPOT
1. Bila board terhubung dengan IP 192.168.1.xxx, board ga bisa nyambung ke websocket server (laptop),
jadi saat IP board 192.168.10.xxx board bisa nyambung ke websocket server.
2. Bila IP laptop 192.168.1.xxx dan board juga 192.168.1.xxx, maka tidak bisa melakukan OTA (over the air) dengan board, jika kedua IP
selain 192.168.1.xxx maka bisa melakukan OTA.
3. IP board 192.168.1.xxx maka tidak bisa buka webpage milik board (Webserver), IP 192.168.10.xxx bisa buka webpage
milik board (Webserver).

Kemarin coba pake AP sendiri (tidak terhubung internet), OTA bisa dilakukan, Websocket bisa terhubung tapi langsung terputus
karena tidak ada internet.

Dugaan masalah berada pada segmen 1 (xxx.xxx.1.xxx) hasil diskusi dengan Pak Abe.
