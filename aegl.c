#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "pic.h"

static uint8_t lcd_trace_porta = 0;
static uint8_t lcd_trace_portb = 0;
static uint8_t lcd_trace_portc = 0;
static uint8_t i2c_trace_trisc = 0;
static int uart_delay = 0;



static void lcd_trace(uint32_t cycle)
{
  bool cs1    = lcd_trace_porta & 0x08;
  bool cs2    = lcd_trace_porta & 0x20;
  bool rw     = lcd_trace_portc & 0x01;
  bool type   = lcd_trace_portc & 0x02;
  bool reset  = lcd_trace_portc & 0x04;
  bool enable = lcd_trace_portc & 0x20;

  fprintf(stdout, "LCD | %08x %s %s %s %s %s %s %02x\n",
    cycle,
    cs1    ? "-  "   : "CS1",
    cs2    ? "-  "   : "CS2",
    reset  ? "Rst"   : "-  ",
    enable ? "En"    : "- ",
    rw     ? "Read " : "Write",
    type   ? "Data"  : "Cmd ",
    lcd_trace_portb);
}



static void i2c_trace(uint8_t value, uint32_t cycle)
{
  value &= 0x18;
  if (value != i2c_trace_trisc) {
    i2c_trace_trisc = value;
    bool scl = value & 0x08;
    bool sda = value & 0x10;
    fprintf(stdout, "I2C | %08x %s %s\n",
      cycle,
      scl ? "SCL" : "-  ",
      sda ? "SDA" : "-  ");
  }
}



static void aegl_reg_read(pic_t *pic, uint16_t f)
{
  int c;

  if (f == PIC_REG_PIR1) {
    uart_delay++;
    if (uart_delay > 100) {
      fprintf(stdout, "> ");
      c = fgetc(stdin);
      if (c == EOF) {
        exit(EXIT_SUCCESS);
      } else if (c == '\n') {
        c = '\r'; /* Commands should end with CR. */
      } else if (c == '.') {
        c = 0x1B; /* Convenient way to write the starting escape character. */
      }
      pic->r[PIC_REG_RCREG] = c;
      pic->r[PIC_REG_PIR1] |= 0x20; /* Set RCIF to indicate new data. */
      uart_delay = 0;
    }
  }
}



static void aegl_reg_write(pic_t *pic, uint16_t f)
{
  uint8_t value;

  switch (f) {
  case PIC_REG_TXREG:
    fprintf(stdout, "TXREG | 0x%02x\n", pic->r[f]);
    break;

  case PIC_REG_PORTA:
    value = pic->r[f] & 0x28;
    if (value != lcd_trace_porta) {
      lcd_trace_porta = value;
      lcd_trace(pic->cycle);
    }
    break;

  case PIC_REG_PORTB:
    value = pic->r[f];
    if (value != lcd_trace_portb) {
      lcd_trace_portb = value;
      lcd_trace(pic->cycle);
    }
    break;

  case PIC_REG_PORTC:
    value = pic->r[f] & 0x27;
    if (value != lcd_trace_portc) {
      lcd_trace_portc = value;
      lcd_trace(pic->cycle);
    }
    break;

  case PIC_REG_TRISC:
    i2c_trace(pic->r[f], pic->cycle);
    break;
  }
}



void aegl_init(pic_t *pic)
{
  pic->in_porta = 0x10; /* Set JP1 input to disable DEMO mode. */
  pic->reg_read_hook = aegl_reg_read;
  pic->reg_write_hook = aegl_reg_write;
}



