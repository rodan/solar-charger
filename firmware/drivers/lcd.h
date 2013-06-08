/**********************************************************************************/
/*    Test program for:															  */
/*	  Board: MSP430F5510-STK													  */
/*    Manufacture: OLIMEX                                                   	  */
/*	  COPYRIGHT (C) 2012														  */
/*    Designed by:  Georgi Todorov                                                */
/*    Module Name    :  GDSC-0801WP-01-MENT                                       */
/*    File   Name    :  lcd.h                                                     */
/*    Revision       :  Rev.A                                                     */
/*    Date           :  20.01.2012					                              */
/**********************************************************************************/
#ifndef __LCD_GDSC_0801WP_01_H
#define __LCD_GDSC_0801WP_01_H

/****** 1.All functions prototypes *****/
void Delay(volatile unsigned long delay);
void LCD_Clear(void);
void LCD_SEND_CHR(unsigned char character);
void LCD_SEND_CMD(unsigned char command);
void LCD_Send_Symbol(unsigned char row, unsigned char position,
                     unsigned char *symbol);
void LCD_Send_STR(unsigned char row, char *dataPtr);
void LCD_Send_Long_STR(unsigned char row, unsigned long shift_rate,
                       char *dataPtr);
void LCD_Check_Busy_Flag(void);
void LCD_Read_Symbol(unsigned char row, unsigned char position);
void LCD_Init(void);

/****** 2.All definitions *****/
#define	RS_ON				P1OUT |= BIT1;
#define	RS_OFF				P1OUT &= ~BIT1;
#define	RW_ON				P1OUT |= BIT2;
#define	RW_OFF				P1OUT &= ~BIT2;
#define	EN_ON				P1OUT |= BIT3;
#define	EN_OFF				P1OUT &= ~BIT3;
#define LCD_PWR_E                       P5OUT |= BIT1;
#define LCD_PWR_DIS                     P5OUT &= ~BIT1;
#define LCD_write_reg                   P1OUT = LCD_write | BIT0;       //write LCD data with BIT0 in 1 because of BUT1 pull-up has to be enabled always
#define LCD_read_reg                    LCD_read = P1IN;

extern volatile unsigned char flag_register;

#endif                          //__LCD_GDSC_0801WP_01_H
