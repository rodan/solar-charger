/**********************************************************************************/
/*Test program for:						 	          */
/*    Board: MSP430F5510-STK							  */
/*    Manufacture: OLIMEX                                                   	  */
/*    COPYRIGHT (C) 2012							  */
/*    Designed by:  Georgi Todorov                                                */
/*    Module Name    :  GDSC-0801WP-01-MENT                                       */
/*    File   Name    :  lcd.c                                                     */
/*    Revision       :  Rev.A                                                     */
/*    Date           :  20.01.2012					          */
/**********************************************************************************/

#include "lcd.h"
#include <msp430f5510.h>

volatile unsigned char flag_register = 0x00;

/**********************************************************************************/
/*  Function name: Delay                                                          */
/*  	Parameters                                                                */
/*          Input   :  delay_counter	                                          */
/*          Output  :  No	                                                  */
/*	Action: Simple delay							  */
/**********************************************************************************/
void Delay(volatile unsigned long delay_counter)
{
    while (delay_counter) {
        delay_counter--;
    }
}

/**********************************************************************************/
/*  Function name: LCD_SEND_CHR                                                   */
/*  	Parameters                                                                */
/*          Input   :  character                                                  */
/*          Output  :  No	                                                  */
/*	Action: Send one data byte to LCD					  */
/**********************************************************************************/
void LCD_SEND_CHR(unsigned char character)
{
    unsigned char data = 0;
    unsigned char temp = 0;
    volatile unsigned char LCD_write;
    volatile unsigned char LCD_read;

    data = character & 0xF0;    //get upper nibble
    LCD_read_reg;               //LCD_read = P1IN;
    temp = LCD_read & 0x0F;     //Read current port state
    LCD_write = data | temp;    //send data to LCD 
    LCD_write_reg;              //P1OUT = LCD_write | BIT0  - set BUT1 pull up resistor
    RW_OFF;                     //set LCD to write
    RS_ON;                      //set LCD to data mode
    EN_ON;                      //toggle E for LCD
    Delay(5);                   //5us
    EN_OFF;
    data = character & 0x0F;
    data = data << 4;           //set D3-D0
    data = data & 0xF0;
    LCD_read_reg;               //LCD_read = P1IN;
    temp = LCD_read & 0x0F;     // Read current port state
    LCD_write = data | temp;    //send data to LCD like no change other Port bits
    LCD_write_reg;              //P1OUT = LCD_write | BIT0  - set BUT1 pull up resistor
    RW_OFF;                     //set LCD to write
    RS_ON;                      //set LCD to data mode
    EN_ON;                      //toggle E for LCD
    Delay(5);                   //5us
    EN_OFF;
    Delay(500);
}

/**********************************************************************************/
/*  Function name: LCD_SEND_CMD                                                   */
/*  	Parameters                                                                */
/*          Input   :  command                                                    */
/*          Output  :  No	                                                  */
/*	Action: Send one command byte to LCD					  */
/**********************************************************************************/
void LCD_SEND_CMD(unsigned char command)
{
    unsigned char data = 0;
    unsigned char temp = 0;
    volatile unsigned char LCD_write;
    volatile unsigned char LCD_read;

    Delay(500);
    data = command & 0xF0;      //get upper nibble
    LCD_read_reg;               //LCD_read = P1IN;
    temp = LCD_read & 0x0F;     // Read current port state
    LCD_write = data | temp;    //send data to LCD like no change other Port bits
    LCD_write_reg;              //P1OUT = LCD_write | BIT0  - set BUT1 pull up resistor
    RW_OFF;                     //set LCD to write
    RS_OFF;                     //set LCD to data mode
    EN_ON;                      //toggle E for LCD
    Delay(5);                   //5us
    EN_OFF;
    data = command & 0x0F;
    data = data << 4;           //set D3-D0
    data = data & 0xF0;
    LCD_read_reg;               //LCD_read = P1IN;
    temp = LCD_read & 0x0F;     // Read current port state
    LCD_write = data | temp;    //send data to LCD like no change other Port bits
    LCD_write_reg;              //P1OUT = LCD_write | BIT0  - set BUT1 pull up resistor
    RW_OFF;                     //set LCD to write
    RS_OFF;                     //set LCD to data mode
    EN_ON;                      //toggle E for LCD
    Delay(5);                   //5us
    EN_OFF;
//    Delay(10000);
    Delay(100);
}

/**********************************************************************************/
/*  Function name: LCD_Send_Symbol                                                */
/*  	Parameters                                                                */
/*          Input   :  row, position, *symbol                                     */
/*          Output  :  No 	                                                  */
/*	Action: Send one symbol to LCD. 					  */
/*			Place of symbol is defined by selected row and position.  */
/**********************************************************************************/
void LCD_Send_Symbol(unsigned char row, unsigned char position,
                     unsigned char *symbol)
{
    if (row > 2)
        return;
    //Set row
    if (row == 1)
        LCD_SEND_CMD(0x80);     // Set DDRAM address 0x00       
    else
        LCD_SEND_CMD(0xC0);     // Set DDRAM address 0x40
    //Set symbol position
    if (position > 8)
        return;
    else {
        LCD_SEND_CMD(0x02);     // Return Home
        while (position) {
            LCD_SEND_CMD(0x14); // Cursor or display shift (Shift cursor to the right, AC is increased by 1)
            position--;
        }
    }
    //Write symbol
    LCD_SEND_CHR(*symbol);

}

/**********************************************************************************/
/*  Function name: LCD_Send_STR                                                   */
/*  	Parameters                                                                */
/*          Input   :  row, *dataPtr                                      	  */
/*          Output  :  No		                                          */
/*	Action: Send string to LCD. Sring size is limited to 8 symbols!		  */
/**********************************************************************************/
void LCD_Send_STR(unsigned char row, char *dataPtr)
{
    unsigned char number_sent_symbols = 0;

    if (row > 2)
        return;
    // Init LCD before write
    LCD_SEND_CMD(0x01);         // Clear Display
    LCD_SEND_CMD(0x02);         // Return home, this instructions clear Display
    LCD_SEND_CMD(0x80);         // Set DDRAM address 0x00
    // End Init
    if (row == 1)
        LCD_SEND_CMD(0x80);     // Set DDRAM address 0x00       
    else
        LCD_SEND_CMD(0xC0);     // Set DDRAM address 0x40
    while (*dataPtr) {
        LCD_SEND_CHR(*dataPtr);
        dataPtr++;
        number_sent_symbols++;
        if (number_sent_symbols > 7)
            return;
    }
}

/**********************************************************************************************/
/*  Function name: LCD_Send_Long_STR                                                          */
/*  	Parameters                                                                            */
/*          Input   :  row, shift_rate, *dataPtr                           	              */
/*          Output  :  No	                                                              */
/*	Action: Send long string to LCD. String size is not limited!		              */
/*			At begin first 8 symbols of the string are visualized at LCD.	      */
/*			The other symbols are visualized like shift string to the right with  */
/*			speed dependent on "shift_rate" variable value.	                      */
/**********************************************************************************************/
void LCD_Send_Long_STR(unsigned char row, unsigned long shift_rate,
                       char *dataPtr)
{
    unsigned char right_shifts = 7;
    unsigned char shift_counter = 0;
    unsigned char LCD_last_8_symbols_backup[8];
    unsigned char mass_index = 0;
    unsigned char i;

    if (row > 2)
        return;
    // Init LCD before write
    LCD_SEND_CMD(0x01);         // Clear Display
    LCD_SEND_CMD(0x02);         // Return home, this instructions clear Display
    LCD_SEND_CMD(0x80);         // Set DDRAM address 0x00
    // End Init
    if (row == 1)
        LCD_SEND_CMD(0x88);     // Set DDRAM address 0x08       
    else
        LCD_SEND_CMD(0xC8);     // Set DDRAM address 0x48
    while (*dataPtr) {
        LCD_SEND_CHR(*dataPtr);
        if (flag_register & 0x88) {
            return;
        }
        Delay(shift_rate);
        LCD_SEND_CMD(0x18);     // Display shift to the right
        shift_counter++;
        if (shift_counter > 22) {
            LCD_last_8_symbols_backup[mass_index] = *dataPtr;   // Save current symbol in backup massive
            mass_index++;
            if (mass_index == 8)
                mass_index = 0;
        }
        if (shift_counter == 30) {
            LCD_SEND_CMD(0x02); // Return home, this instructions clear Display, so we have to restore it!
            LCD_SEND_CMD(0x80); // Set DDRAM address 0x00
            for (i = 0; i < 8; i++) {
                LCD_SEND_CHR(LCD_last_8_symbols_backup[i]);
            }
            shift_counter = 0;
        }
        dataPtr++;
    }
    while (right_shifts) {
        LCD_SEND_CHR(0x20);     // Send empty symbol to LCD
        LCD_SEND_CMD(0x18);     // Display shift to the right
        Delay(shift_rate);
        if (shift_counter == 30) {
            LCD_SEND_CMD(0x02); // Return home
            LCD_SEND_CMD(0x80); // Set DDRAM address 0x00
            for (i = 0; i < 8; i++) {
                LCD_SEND_CHR(LCD_last_8_symbols_backup[i]);
            }
            shift_counter = 0;
        }
        right_shifts--;
        if (right_shifts == 0) {
            LCD_SEND_CMD(0x01); // Clear Display
            LCD_SEND_CMD(0x02); // Return home, this instructions clear Display, so we have to restore it!
            LCD_SEND_CMD(0x80); // Set DDRAM address 0x00
        }
    }
}

/**********************************************************************************/
/*  Function name: LCD_Clear                                                      */
/*  	Parameters                                                                */
/*          Input   :  No       		                               	  */
/*          Output  :  No	                                                  */
/*	Action: Clear entire display						  */
/**********************************************************************************/
void LCD_Clear(void)
{
    LCD_SEND_CMD(0x01);         // ClearDisplay
}

/**********************************************************************************/
/*  Function name: LCD_Check_Busy_Flag                                            */
/*  	Parameters                                                                */
/*          Input   :  No       		                               	  */
/*          Output  :  No	                                                  */
/*	Action: Check is the LCD driver in busy state. Not used with this LCD!!!  */
/**********************************************************************************/
void LCD_Check_Busy_Flag(void)
{
    unsigned char LCD_read = 0x02;
    unsigned char busy_flag = 0x02;
    unsigned char AC_address;

    RW_ON;
    RS_OFF;
    while (busy_flag) {
        EN_ON;                  //toggle E for LCD
        Delay(5);               //5us
        EN_OFF;
        LCD_read_reg;
        busy_flag = LCD_read & 0x80;    // Check busy flag
        AC_address = LCD_read & 0x70;   // get high byte of address
        EN_ON;                  //toggle E for LCD
        Delay(5);               //5us
        EN_OFF;
        LCD_read_reg;
        LCD_read = LCD_read >> 4;
        AC_address = AC_address | LCD_read;     //Get full address
// This LCD return two times the AC_address(once in high byte and once in low byte), 
// because address is in range (0-7, i.e. 8 characters)!!!!!!
    }
}

/**********************************************************************************/
/*  Function name: LCD_Read_Symbol                                                */
/*  	Parameters                                                                */
/*          Input   :  row, position     		                          */
/*          Output  :  No                                                   	  */
/*	Action: Read character from LCD's DDRAM. 	  			  */
/*			DDRAM address is defined with "row" and "position" 	  */
/*			This command is not support of this LCD driver!!! 	  */
/**********************************************************************************/
void LCD_Read_Symbol(unsigned char row, unsigned char position)
{
    volatile unsigned char LCD_read;
    volatile unsigned char Read_data;

    if (row > 2)
        return;
    //Set row
    if (row == 1)
        LCD_SEND_CMD(0x80);     // Set DDRAM address 0x00       
    else
        LCD_SEND_CMD(0xC0);     // Set DDRAM address 0x40
    //Set symbol position
    if (position > 8)
        return;
    else {
        LCD_SEND_CMD(0x02);     // Return Home
        while (position) {
            LCD_SEND_CMD(0x14); // Cursor or display shift (Shift cursor to the right, AC is increased by 1)
            position--;
        }
    }
    //Read symbol
    RS_ON;
    Delay(5);
    RW_ON;
    Delay(5);
    EN_ON;                      //toggle E for LCD
    Delay(5);                   //5us
    EN_OFF;
    Delay(5);
    LCD_read_reg;
    LCD_read = LCD_read & 0xF0; //Get high byt half
    Read_data = LCD_read;
    EN_ON;                      //toggle E for LCD
    Delay(5);                   //5us
    EN_OFF;
    LCD_read_reg;
    LCD_read = LCD_read >> 4;   //Get low byt half
    Read_data = Read_data | LCD_read;   // Check Data
}

/**********************************************************************************/
/*  Function name: LCD_Init		                                          */
/*  	Parameters                                                                */
/*          Input   :  No			     		                  */
/*          Output  :  No                                                   	  */
/*	Action: Initialize LCD driver. 	  					  */
/**********************************************************************************/
void LCD_Init(void)
{
//LCD pin CSB have to be in low state before initialization!
    unsigned char LCD_DATA_PORT_temp = 0;
    volatile unsigned char LCD_write;
    volatile unsigned char LCD_read;

    LCD_PWR_E;                  //enable LCD power

// This first commands have 4 bits Length!!!!! Not Byte!!!
    Delay(50000);               // TIME>15ms
    RS_OFF;
    RW_OFF;
    LCD_read_reg;               //LCD_read = P1IN;
    LCD_DATA_PORT_temp = LCD_read & 0x0F;
    LCD_write = LCD_DATA_PORT_temp | 0x30;
    LCD_write_reg;              //P1OUT = LCD_write | BIT0  - set BUT1 pull up resistor
    EN_ON;
    Delay(5);
    EN_OFF;
    Delay(5);

    Delay(500);                 // TIME>4,1ms
    LCD_read_reg;
    LCD_DATA_PORT_temp = LCD_read & 0x0F;
    LCD_write = LCD_DATA_PORT_temp | 0x30;
    LCD_write_reg;
    EN_ON;
    Delay(5);
    EN_OFF;
    Delay(5);

    Delay(500);                 // Time>100us
    LCD_read_reg;
    LCD_DATA_PORT_temp = LCD_read & 0x0F;
    LCD_write = LCD_DATA_PORT_temp | 0x30;
    LCD_write_reg;
    EN_ON;
    Delay(5);
    EN_OFF;
    Delay(5);

    Delay(500);
    LCD_read_reg;
    LCD_DATA_PORT_temp = LCD_read & 0x0F;
    LCD_write = LCD_DATA_PORT_temp | 0x20;      // Function Set
    LCD_write_reg;
    EN_ON;
    Delay(5);
    EN_OFF;
    Delay(5);

    // Next commands have 8 bits Length i.e. one Byte !!!
    LCD_SEND_CMD(0x28);         // FUNCTION SET
    LCD_SEND_CMD(0x08);         // DISPLAY OFF
    LCD_SEND_CMD(0x01);         // DISPLAY CLEAR
    LCD_SEND_CMD(0x06);         //Entry mode set
    LCD_SEND_CMD(0x0C);         //DISPLAY ON                   
}
