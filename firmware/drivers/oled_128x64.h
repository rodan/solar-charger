/*
  SeeedOLED.h - SSD130x OLED Driver Library
  2011 Copyright (c) Seeed Technology Inc.  All right reserved.
 
  Author: Visweswara R
  
  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef __seeedOLED_H__
#define __seeedOLED_H__

#include <inttypes.h>

#define SeeedOLED_Max_X 		127     //128 Pixels
#define SeeedOLED_Max_Y 		63      //64  Pixels

#define PAGE_MODE			01
#define HORIZONTAL_MODE			02

#define SeeedOLED_Address		0x3c
#define SeeedOLED_Command_Mode		0x80
#define SeeedOLED_Data_Mode		0x40
#define SeeedOLED_Display_Off_Cmd	0xAE
#define SeeedOLED_Display_On_Cmd	0xAF
#define SeeedOLED_Normal_Display_Cmd	0xA6
#define SeeedOLED_Inverse_Display_Cmd	0xA7
#define SeeedOLED_Activate_Scroll_Cmd	0x2F
#define SeeedOLED_Dectivate_Scroll_Cmd	0x2E
#define SeeedOLED_Set_Brightness_Cmd	0x81

#define Scroll_Left			0x00
#define Scroll_Right			0x01

#define Scroll_2Frames			0x7
#define Scroll_3Frames			0x4
#define Scroll_4Frames			0x5
#define Scroll_5Frames			0x0
#define Scroll_25Frames			0x6
#define Scroll_64Frames			0x1
#define Scroll_128Frames		0x2
#define Scroll_256Frames		0x3

uint8_t addressing_mode;

uint8_t oled_128x64_init(void);
uint8_t oled_128x64_send_command(const uint8_t command);
uint8_t oled_128x64_send_data(const uint8_t data);

void oled_128x64_clear_display(void);
void oled_128x64_set_brightness(const uint8_t brightness);
void oled_128x64_set_horizontal_mode(void);
void oled_128x64_set_page_mode(void);
void oled_128x64_set_text_xy(const uint8_t row, const uint8_t column);
void oled_128x64_put_char(uint8_t ch);
void oled_128x64_put_string(const char *string);
uint8_t oled_128x64_put_number(uint32_t long_num);
uint8_t oled_128x64_put_float(float floatNumber, uint8_t decimal);
void oled_128x64_draw_bitmap(const uint8_t * bitmaparray, const uint16_t bytes);
void oled_128x64_set_horizontal_scroll_properties(const uint8_t direction,
                                                  const uint8_t start_page,
                                                  const uint8_t end_page,
                                                  const uint8_t scroll_speed);
void oled_128x64_activate_scroll(void);
void oled_128x64_deactivate_scroll(void);
void oled_128x64_set_normal_display(void);
void oled_128x64_set_inverse_display(void);

#endif
