# WSPR_GPS_Beacon
This is a single-band WSPR beacon that uses an si5351 breakout board for frequency synthesis and a Ublox 6M serial GPS unit for time synchronisation. 
This project was published in short form in the RSGB QRP Club's SPRAT, issue 192, Autumn 2022.

For a Blog post write-up see: https://vk3hn.wordpress.com/2021/10/04/20-meters-200mw-12000-miles-wspr-magic/

The WSPR control code was sourced from Zachtek at  https://github.com/HarrydeBug/1011-WSPR-TX_LP1

Libraries:
#include <SoftwareSerial.h>
#include <LiquidCrystal.h>
#include <si5351.h>     // by NT7S https://github.com/etherkit/Si5351Arduino
#include <JTEncode.h>   // by NT7S https://github.com/etherkit/JTEncode

# Steps to get this going
Since publishing this project in SPRAT I have received quite a lot of enquiries from people who have not done much 
with Arduino projects or software.  Here are some notes that I have found myself repeatedly typing. If you have 
not done much with Arduinos, I recommend you follow these steps. If you still have problems, then email me.

The first thing to do is to make sure you have the JTEncode and si5351 libraries installed and working correctly with your breakout boards.
The JTEncode library can be found here: https://github.com/etherkit/JTEncode
The si5351 library can be found here:   https://github.com/etherkit/Si5351Arduino

The above URLs will take you to the source, but I find the easiest way to install is using the Arduino IDS's library manager. 
Open Library Manager, and search for si5351 (JTEncode) and find the NT7S library, then click on install.

Start with the si5351.  After you have the library installed, connect up your si5351 to Uno or Nano (using the usual I2C lines on A4 and A5) 
and run one of the library's example programs to verify that your microcontroller is talking to your si5351 and is generating a signal.
Example scripts are in the Examples folder in the installed library.
Read the comments in the example script to see how it works, and what, if anything, you need to set to get a measurable result.  

Remember to open the console, and make sure the speed (baud rate) passed into the Serial() object in the script you are running matches the 
speed/baud rate setting in the console window.  The console window will trace out values from the running script that will ensure your script
is actually executing, and is doing what you expect it to.  

Once you have the si5351 working, install and test the JTEncode library in a similar way. Jason NT7S has a neat test script that you can run at
https://github.com/etherkit/JTEncode/blob/master/examples/Si5351JTDemo/Si5351JTDemo.ino

It is capable of producing the frequencies for JT65/JT9/JT4/FT8/WSPR/FSQ (set the desired mode at line 172).   You will need to connect a momentary
push button to pin 12 to use as the transmit trigger.  See Jason's notes in the header.  If you have the si5351 connected, you will get a burst of
the selected mode at the default frequency (set at lines 63 to 68) from your si5351. 

The UBlox unit is a little different, there is no need for a library, the Arduino talks to it using a SoftwareSerial connection. Initially, I tried
using TinyGPS to do the NMEA string decodes, but I removed this to save memory, and because I only needed the timestamp information.  So I wrote
simple string parsing code to dig out the message header and timestamp field.  

To test the UBlox board, you can use any of the GPS scripts on the internet, or you can load up my beacon script, run it, and look for the 
NMEA strings to be traced out on the console.  

The UBlox 6M unit flashes its LED when it has detected a few satellites. If you run it for more than a minute and you don't get any flashing, 
or NMEA strings on the console, try moving the whole thing to a spot where it has a fighting chance of receiving satellites, so on a window ledge,
or better still, outside.  My UBlox does not decode anything on the shack bench which is under a metal roof, but does work next to the window.    

# Help
If after all of this you are still having problems, you are welcome to email me. Good luck!   
