# WSPR_GPS_Beacon
This is a single-band WSPR beacon that uses an si5351 breakout board for frequency synthesis and a Ublox 6M serial GPS unit for time synchronisation. 

For a Blog post write-up see: https://vk3hn.wordpress.com/2021/10/04/20-meters-200mw-12000-miles-wspr-magic/

The WSPR control code was sourced from Zachtek at  https://github.com/HarrydeBug/1011-WSPR-TX_LP1

Libraries:
#include <SoftwareSerial.h>
#include <LiquidCrystal.h>
#include <si5351.h>     // by NT7S https://github.com/etherkit/Si5351Arduino
#include <JTEncode.h>   // by NT7S https://github.com/etherkit/JTEncode
