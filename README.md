## PicoGBcart

Emulates a gameboy cart with a Raspberry Pi Pico for use on a Gameboy, Gameboy Color or Gameboy Advance in GBC mode. GBC code at this point isn't supported mostly because of speed issues which may not be able to be overcome. This is an issue with the speed of QSPI.

Support for MBC1 and MBC5 only for now. For a 2MB pico only a one megabyte rom will fit. Not everything is implemented or supported so milage WILL vary.

This was connected into a GBA SP and an original DMG-01 to test without level adapters. It works but is extremely power hungry and required I use external power on the DMG-01.

## How to Use

If you plan to use a DMG-01 or color gameboy, chip select will need to be connected to pin 25 and ignore_CS value set to 0.  Should work on a GBA or GBA SP without chip select connected.

Throw in rom data at offset 0x20000 and, if you have sram data, place it at offset 0x18000 (to load save data). Saving to flash is currently not implemented mostly because finding a sane time to save the game is troublesome without crashing to rom. 

Microsoft offers a tool for building UF2s for flashing.

## IDEAS AND TODOS

-add images of hardware connection
- implement saves
- find a way to get GBC code to work. 

## Update 3-24-2024 
There has been significant progress with this project in the last month. I've been operating under the assumption the picos GPIOs near 5V tolerant, with the exception of the ADC gpio pins, as per these articles
https://hackaday.com/2023/04/05/rp2040-and-5v-logic-best-friends-this-fx9000p-confirms/
https://forums.raspberrypi.com/viewtopic.php?p=2092122#p2091977

I have been using a voltage divider on the two ADC pins I do need to use for signals from the gameboy, which will subject the pin on the gameboy as well as the picos GPIO pin to around 1.8mA. Although this has worked fine for me I want to verify that this is indeed safe enough for daily driving. I am aware of safer ways to do this however my endgoal is to try to make this as easy to assemble as possible with as few parts as needed, with the exception of needing to make/order a PCB.

I have managed to eliminate the need for the CS pin, being able to keep track of timing with A15. This has allowed me to use one pin to tell the pico to save the SRAM of anything that might make use of it.

Given the Pico has to read from QSPI for larger programs to serve to the Gameboy, there is a good chance Gameboy Color is too fast for it. This has been mostly affirmed to me by other peoples struggles with this as well.
Some software for the Gameboy is able to work on original DMG-01 but take advantage of the faster hardware on GBC and on GBA in GBC mode. I have come up with a way to mitigate this issue on faster hardware by tricking the game into thinking its on original hardware.

I will be working on make a kicad model for the PCB based on some prototype work I had made using inkscape and my CNC machine. I also have a STL case in the works which can house the assembled PCB.

Updates will be slow but I am moving forward.
