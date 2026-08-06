# Lantern-RGB-lamp
RGB lamp inspired by the minecraft lantern
parts: 
Esp32 c3 supermini (clone from china bought for cheap)
ws2812b rgb led matrix 5x5 version
ttp223 touch button module used for brightness controll/turning off
the whole lantern was printed with my a1 mini, no ams.
its separated into 3 main sections - the base and top wich are  were printed black and 4 pillars wich were printed brown. the pillars should have a fit thats on the tighter side both to the top and the base. 

## WiFi setup (local only)
1. Copy `/home/runner/work/Lantern-RGB-lamp/Lantern-RGB-lamp/main/wifi_secrets.example.h` to `/home/runner/work/Lantern-RGB-lamp/Lantern-RGB-lamp/main/wifi_secrets.h`.
2. Set your local SSID and password in `wifi_secrets.h`.

`wifi_secrets.h` is ignored by git so credentials stay out of commits.