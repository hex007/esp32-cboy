/*************************************************************************
 *   Cboy, a Game Boy emulator
 *   Copyright (C) 2014 jkbenaim
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 ************************************************************************/

#include "memory.h"
#include "cart_chardev.h"
#include "cart.h"
#include "mbc_c_mbc2.h"

int rom_bank_shadow;
int ram_bank_shadow;

void mbc_c_mbc2_install()
{
  int i;
  
  // invalidate caches
  for( i=0; i<8*1024*1024; ++i )
  {
    cart.cartromValid[i] = 0;
  }
  for( i=0; i<cart.extram_size; ++i )
  {
    cart.extramValidRead[i] = 0;
    cart.extramValidWrite[i] = 0;
  }
  
  // read cart bank zero
  for( i=0x0; i<=(0x3F); ++i ) {
    readmem[i]   = mbc_c_mbc2_read_bank_0;
  }
  // read cart bank n
  for( i=0x40; i<=(0x7F); ++i ) {
    readmem[i]   = mbc_c_mbc2_read_bank_n;
  }
  
  // write 0000-1FFF: ram enable
  for( i=0x00; i<=0x1F; ++i ) {
    writemem[i] = mbc_c_mbc2_write_ram_enable;
  }
  // write 2000-3FFF: rom bank select
  for( i=0x20; i<=0x3F; i+=2 ) {
    writemem[i] = mbc_c_mbc2_write_dummy;
    writemem[i+1] = mbc_c_mbc2_write_rom_bank_select;
  }
  // write 4000-5FFF: nothing
  for( i=0x40; i<=0x5F; ++i ) {
    writemem[i] = mbc_c_mbc2_write_dummy;
  }
  // write 6000-7FFF: nothing
  for( i=0x60; i<=0x7F; ++i ) {
    writemem[i] = mbc_c_mbc2_write_dummy;
  }
  
  // read A000-A1FF: read extram
  for( i=0xA0; i<0xA2; ++i ) {
    readmem[i] = mbc_c_mbc2_read_extram;
  }
  for( i=0xA2; i<=0xBF; ++i ) {
    readmem[i] = mbc_c_mbc2_read_ff;
  }
  
  // write A000-A1FF: write extram
  for( i=0xA0; i<0xA2; ++i ) {
    writemem[i] = mbc_c_mbc2_write_extram;
  }
  for( i=0xA2; i<=0xBF; ++i ) {
    writemem[i] = mbc_c_mbc2_write_dummy;
  }
  
  // set up cart params
  cart.cartrom_bank_zero = cart.cartrom;
  cart.cartrom_bank_n = cart.cartrom + 0x4000;
  
  // enable extram, just in case
  ca_write( cart.fd, 0x0000, 0x00 );
  
  rom_bank_shadow = -1;
  cart.cart_bank_num = 1;
  ram_bank_shadow = -1;
  cart.extram_bank_num = 0;
  
  cart.cleanup = mbc_c_mbc2_cleanup;
}

uint8_t mbc_c_mbc2_read_bank_0( uint16_t address )
{
  if( !cart.cartromValid[address] )
  {
    // fill cache
    unsigned int startAddress = address & 0xFF00;
    ca_read256Bytes( cart.fd, startAddress, cart.cartrom+startAddress );
    
    // mark cache as valid
    int i;
    for( i=0; i<256; i++ )
      cart.cartromValid[startAddress+i] = 1;
  }
  // read from cache
  return cart.cartrom[address];
}

uint8_t mbc_c_mbc2_read_bank_n( uint16_t address ) {
  if( !cart.cartromValid_bank_n[address-0x4000] )
  {
    // set rom bank
    if( rom_bank_shadow != cart.cart_bank_num )
    {
      ca_write( cart.fd, 0x2100, cart.cart_bank_num );
      rom_bank_shadow = cart.cart_bank_num;
    }
    // fill cache
    unsigned int startAddress = address & 0xFF00;
    ca_read256Bytes( cart.fd, startAddress, cart.cartrom_bank_n+startAddress-0x4000 );
    
    // mark cache as valid
    int i;
    for( i=0; i<256; i++ )
      cart.cartromValid_bank_n[startAddress+i-0x4000] = 1;
  }
  // read from cache
  return cart.cartrom_bank_n[address-0x4000];
}

void mbc_c_mbc2_write_dummy( uint16_t address, uint8_t data ) {
  // do nothing
}

uint8_t mbc_c_mbc2_read_ff( uint16_t address ) {
  return 0xFF;
}

// write 0000-1FFFF: ram enable
void mbc_c_mbc2_write_ram_enable( uint16_t address, uint8_t data ) {
  // MBC2 is weird.
  // RAM is enabled when the least significant bit of the upper address byte is 0.
  // For example, writing any value to address 0x0000 would enable RAM.
  // Likewise, writing any value to address 0x0100 would disable RAM.
  cart.extramEnabled = (address & 0x0100) >> 8;
}

// write 2000-3FFF: rom bank select
void mbc_c_mbc2_write_rom_bank_select( uint16_t address, uint8_t data ) {
  size_t offset;
  data &= 0x0F;	// only 16 banks are supported
  if(data == 0)
    data = 1;
  cart.cart_bank_num = data;
  offset = (size_t)data*16384 % cart.cartromsize;
  
  cart.cartrom_bank_n = cart.cartrom + offset;
  cart.cartromValid_bank_n = cart.cartromValid + offset;
}


// read A000-BFFF extram
uint8_t mbc_c_mbc2_read_extram( uint16_t address ) {
  if( (cart.extramEnabled&0x01) == 0x01 )
  {
    return 0xff;
  }
  if( !cart.extramValidRead[address-0xA000] )
  {
    // fill cache
    unsigned int startAddress = address & 0xFF00;
    uint8_t buf[256];
    ca_read256Bytes( cart.fd, startAddress, buf );
    int i;
    for( i=0; i<256; i++ )
    {
      if( cart.extramValidRead[startAddress+i-0xA000] == 0 )
	cart.extram[startAddress+i-0xA000] = buf[i];
    }
    
    // mark cache as valid
    for( i=0; i<256; i++ )
      cart.extramValidRead[startAddress+i-0xA000] = 1;
  }
  // read from cache
  return cart.extram[address - 0xA000];
}

// write A000-BFFF extram
void mbc_c_mbc2_write_extram( uint16_t address, uint8_t data ) {
  // MBC2 has 512x4bits of RAM built into the MBC2 itself (no external RAM).
  // Only the lower 4 bits of the byte are stored. Upper 4 bits read as 0.
  data &= 0x0f;
  cart.extram[address&0x01ff] = data;
  cart.extramValidRead[address&0x01ff] = 1;
  cart.extramValidWrite[address&0x01ff] = 1;
}

void mbc_c_mbc2_cleanup() {
  // save extram back
  
  if( cart.battery_backed )
  {
    // enable extram
    ca_write( cart.fd, 0x0000, 0x00 );
    
    // write to extram
    unsigned int address;
    unsigned char data;
    for( address=0xA000; address<0xA200; address++ )
    {
      if(cart.extramValidWrite[address-0xA000] == 1)
      {
	data = cart.extram[address-0xA000];
	ca_write( cart.fd, address, data );
      }
    }
  }
}

