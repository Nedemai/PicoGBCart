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
