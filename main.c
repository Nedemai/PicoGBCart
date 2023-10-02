
/*
 * The MIT License (MIT)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "pin_layout.h"
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/vreg.h"
#include "hardware/flash.h"
#include "hardware/sync.h"

#define FLASH_TARGET_OFFSET_RAM (0x18000) // offset location of SRAM saved to Flash
#define FLASH_TARGET_OFFSET (0x20000) // offset of rom data in flash

const uint8_t *flash_loc_ram = (const uint8_t *)(XIP_NOALLOC_BASE + FLASH_TARGET_OFFSET_RAM); // flash_loc_ram[i] for reading
const uint8_t *flash_loc = (const uint8_t *)(XIP_NOALLOC_BASE + FLASH_TARGET_OFFSET); // flash_loc[i] for reading

// RAM declared as global to make all functions able to access. There probably is better ways
uint8_t Mempak[32769]; // first 2 banks 0,1 or full 32kb rom
uint8_t GB_RAM[32769]; // supports up to 4 banks of 8kb right now

void setup_gpio_pins();
void a32kb_ROM();
void a_no_RAM_ROM();
void a_yes_RAM_ROM();

//--- CORE 0 -----------------------------------------------------------
int main() 
{
  vreg_set_voltage(VREG_VOLTAGE_1_20);
  set_sys_clock_khz(284000, true);
  setup_gpio_pins();
//Populate SRAM and first Bank into the Picos Ram, could DMA this but doesnt matter
  for (int i = 0; i <= (32768); i++)
  {
    GB_RAM[i] = flash_loc_ram[i];                           
  }
  for (int i = 0; i <= (32768); i++)
  {
    Mempak[i] = flash_loc[i];
  }

  int ignore_CS = 1;//incase you dont want to connect Chip Select, only works on GBA in GBC mode
  int checksum = Mempak[0x14E] + (Mempak[0x14F] << 8); // gives unique identifier for ROM
   int rom_size_kb = Mempak[0x148]; //if > 0, BANKED
  int ram_size = Mempak[0x149]; // if > 0, RAM

  // SPECIAL HARDWARE CONDITIONS
  switch (checksum)
  {
  case 0xF9E0: // SML2
    ignore_CS = 1;
    break;
  default:
    break;
  }

  // Bankswitch Scheme
  switch (Mempak[0x147])
  {
  case 0x01: // MBC1
#define MBC1
    break;
  case 0x02: // MBC1 + RAM
  case 0x03: // MBC1 + RAM + BATTERY
#define MBC1 // ram_present = true;
    break;
  case 0x19: // MBC5
#define MBC5
    break;
  case 0x1A: // MBC5 + RAM
  case 0x1B: // MBC5 + RAM + BATTERY
#define MBC5 //  ram_present = true;
    break;
  default:
    break;
  }
  
  gpio_set_dir_out_masked(0x7F8000);

  if (rom_size_kb == 0)
  {
    a32kb_ROM(ignore_CS); // Basic 32kb rom, runs direct from picos ram
  }

  if (ram_size == 0)
  {
    a_no_RAM_ROM(ignore_CS); // For any MBC1 or MBC5 scheme without RAM
  }
  else
  {
    a_yes_RAM_ROM(ignore_CS); // For any MBC1 or MBC5 scheme with RAM
  }

} // MAIN

// RAM BANKED =========================================================================================================================================================
// a_yes_RAM_ROM(IGNORE CHIP SELECT)
void a_yes_RAM_ROM(int ignore_CS) // (IGNORE CHIP SELECT)
{
  int BANK = 0;
  int RAM_BANK = 0;
  int ram_enable = 0;
  int address = 0;
  ;
  while (true)
  {
    gpio_set_dir_in_masked(0x7F8000);
    address = ((sio_hw->gpio_in) & 0x7FFF);
    if (gpio_get(CS) || ignore_CS)
    {
      if (!gpio_get(RD)) // READ ---------------------------------------------------------------------------------------------------------------
      {
        address = ((sio_hw->gpio_in) & 0x7FFF);
        gpio_set_dir_out_masked(0x7F8000);
        if (gpio_get(A15))
        {
          gpio_put_masked(0x7F8000, GB_RAM[address + (RAM_BANK << 13)] << 15);
        }
        else if (address & 0x4000)
        {
          address &= 0x3FFF;
          gpio_put_masked(0x7F8000, flash_loc[address + (BANK * 0x4000)] << 15);
        }
        else // CGB soeed is an issue
        {
          gpio_put_masked(0x7F8000, Mempak[address] << 15);
        }
      }//RD
      if (!gpio_get(WR)) // WRITE ---------------------------------------------------------------------------------------------------------------
      {
        address = ((sio_hw->gpio_in) & 0x7FFF);
        gpio_set_dir_in_masked(0x7F8000);
        if (gpio_get(A15))
        {
          if (ram_enable == 0x0A)
          {
            GB_RAM[address + (RAM_BANK << 13)] = (((sio_hw->gpio_in) & 0x7F8000) >> 15); // optimized for speed
          }
        }
        else
        {
          switch (address & 0xF000)
          {
          case 0x0000:
          case 0x1000: // RAM ENABLE -------
            ram_enable = (((sio_hw->gpio_in) & 0x7F8000) >> 15);
            break;
          case 0x2000: // ROM BANK LOW -------
            BANK = (((sio_hw->gpio_in) & 0x7F8000) >> 15);
#ifdef MBC1
            if (BANK == 0) // MBC1
            {
              BANK = 1;
            }
#endif
            break;
          case 0x3000: // ROM BANK LOW MBC1 ROM BANK HIGH MBC5 ----------
#ifdef MBC5
            BANK = BANK + (((((sio_hw->gpio_in) & 0x7F8000) >> 15) << 4) & 0x10); // MBC5
#endif
#ifdef MBC1
            BANK = (((sio_hw->gpio_in) & 0x7F8000) >> 15); // MBC1
            if (BANK == 0)                                 // MBC1
            {
              BANK = 1;
            }
#endif
            break;
          case 0x4000:
          case 0x5000: // RAM BANK -------
            RAM_BANK = (((sio_hw->gpio_in) & 0x7F8000) >> 15);
            break;
          }//switch
        }//check for A15 (RAM)
      }//WR
    } //CS
  }//While Loop
} //function
// RAM BANKED =========================================================================================================================================================

// NO RAM BANKED =========================================================================================================================================================
// a_no_RAM_ROM(IGNORE CHIP SELECT)
void a_no_RAM_ROM(int ignore_CS) // (IGNORE CHIP SELECT)
{
  int BANK = 0;
  int address = 0;
  while (true)
  {
    gpio_set_dir_in_masked(0x7F8000);
    address = ((sio_hw->gpio_in) & 0x7FFF);
    if (gpio_get(CS) || ignore_CS)
    {
      if (!gpio_get(RD)) // Checks state of READ
      {
        address = ((sio_hw->gpio_in) & 0x7FFF);
        gpio_set_dir_out_masked(0x7F8000);
        if (address & 0x4000)
        {
          address &= 0x3FFF;
          gpio_put_masked(0x7F8000, flash_loc[address + (BANK * 0x4000)] << 15);
        }
        else // CGB soeed is an issue
        {
          gpio_put_masked(0x7F8000, Mempak[address] << 15);
        }
      }
      if (!gpio_get(WR)) // WRITE -------
      {
        address = ((sio_hw->gpio_in) & 0x7FFF);
        gpio_set_dir_in_masked(0x7F8000);
        switch (address & 0xF000)
        {
        case 0x2000: // ROM BANK LOW ------
          BANK = (((sio_hw->gpio_in) & 0x7F8000) >> 15);
#ifdef MBC1
          if (BANK == 0) // MBC1
          {
            BANK = 1;
          }
#endif
          break;
        case 0x3000: // ROM BANK LOW MBC1 ROM BANK HIGH MBC5 ------
#ifdef MBC5
          BANK = BANK + (((((sio_hw->gpio_in) & 0x7F8000) >> 15) << 4) & 0x10); // MBC5
#endif
#ifdef MBC1
          BANK = (((sio_hw->gpio_in) & 0x7F8000) >> 15); // MBC1
          if (BANK == 0)                                 // MBC1
          {
            BANK = 1;
          }
#endif
          break;
        }//switch
      }//WR
    }//CS
  }//While Loop
}//function
// NO RAM BANKED =========================================================================================================================================================

// BASIC 32KB =========================================================================================================================================================
// a32kb_ROM(IGNORE CHIP SELECT)
void a32kb_ROM(int ignore_CS)
{
  int address = 0;
  while (true)
  {
    while (!gpio_get(RD))
    {
      address = ((sio_hw->gpio_in) & 0x7FFF);
      gpio_put_masked(0x7F8000, Mempak[address] << 15); // This runs from ram
      //  gpio_put_masked(0x7F8000, flash_loc[address] << 15);// This runs from ROM and is too slow for GBC
    }
  }
  return;
}
// (IGNORE CHIP SELECT)
// BASIC 32KB =========================================================================================================================================================

void setup_gpio_pins()
{
  // Address pins.
  gpio_init(A0);
  gpio_set_dir(A0, GPIO_IN);
  gpio_init(A1);
  gpio_set_dir(A1, GPIO_IN);
  gpio_init(A2);
  gpio_set_dir(A2, GPIO_IN);
  gpio_init(A3);
  gpio_set_dir(A3, GPIO_IN);
  gpio_init(A4);
  gpio_set_dir(A4, GPIO_IN);
  gpio_init(A5);
  gpio_set_dir(A5, GPIO_IN);
  gpio_init(A6);
  gpio_set_dir(A6, GPIO_IN);
  gpio_init(A7);
  gpio_set_dir(A7, GPIO_IN);
  gpio_init(A8);
  gpio_set_dir(A8, GPIO_IN);
  gpio_init(A9);
  gpio_set_dir(A9, GPIO_IN);
  gpio_init(A10);
  gpio_set_dir(A10, GPIO_IN);
  gpio_init(A11);
  gpio_set_dir(A11, GPIO_IN);
  gpio_init(A12);
  gpio_set_dir(A12, GPIO_IN);
  gpio_init(A13);
  gpio_set_dir(A13, GPIO_IN);
  gpio_init(A14);
  gpio_set_dir(A14, GPIO_IN);

  // Data pins.
  gpio_init(D0);
  gpio_set_dir(D0, GPIO_OUT);
  gpio_set_slew_rate(D0, GPIO_SLEW_RATE_FAST);
  gpio_init(D1);
  gpio_set_dir(D1, GPIO_OUT);
  gpio_set_slew_rate(D1, GPIO_SLEW_RATE_FAST);
  gpio_init(D2);
  gpio_set_dir(D2, GPIO_OUT);
  gpio_set_slew_rate(D2, GPIO_SLEW_RATE_FAST);
  gpio_init(D3);
  gpio_set_dir(D3, GPIO_OUT);
  gpio_set_slew_rate(D3, GPIO_SLEW_RATE_FAST);
  gpio_init(D4);
  gpio_set_dir(D4, GPIO_OUT);
  gpio_set_slew_rate(D4, GPIO_SLEW_RATE_FAST);
  gpio_init(D5);
  gpio_set_dir(D5, GPIO_OUT);
  gpio_set_slew_rate(D5, GPIO_SLEW_RATE_FAST);
  gpio_init(D6);
  gpio_set_dir(D6, GPIO_OUT);

  gpio_set_slew_rate(D6, GPIO_SLEW_RATE_FAST);
  gpio_init(D7);
  gpio_set_dir(D7, GPIO_OUT);
  gpio_set_slew_rate(D7, GPIO_SLEW_RATE_FAST);

  // Control Pins.
  gpio_init(A15); // ADDRESS 15, used for saves
  gpio_set_dir(A15, GPIO_IN);
  gpio_set_slew_rate(A15, GPIO_SLEW_RATE_FAST);

  gpio_init(RD); // WRITE ENABLE
  gpio_set_dir(RD, GPIO_IN);
  gpio_set_slew_rate(RD, GPIO_SLEW_RATE_FAST);

  gpio_init(WR); // OUTPUT ENABLE
  gpio_set_dir(WR, GPIO_IN);
  gpio_set_slew_rate(WR, GPIO_SLEW_RATE_FAST);

  gpio_init(25); // LED (used for chip select)
  gpio_set_dir(25, GPIO_OUT);
}
