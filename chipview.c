#include "chipview.h"
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <curses.h>

#include "pic.h"



static bool pic_port_direction(pic_t *pic, char port, int bit)
{
  switch (port) {
  case 'A':
    return (pic->r[PIC_REG_TRISA] >> bit) & 1;
  case 'B':
    return (pic->r[PIC_REG_TRISB] >> bit) & 1;
  case 'C':
    return (pic->r[PIC_REG_TRISC] >> bit) & 1;
  case 'D':
    return (pic->r[PIC_REG_TRISD] >> bit) & 1;
  case 'E':
    return (pic->r[PIC_REG_TRISE] >> bit) & 1;
  default:
    return false;
  }
}



static char *right_side_direction(pic_t *pic, char port, int bit)
{
  if (pic_port_direction(pic, port, bit)) {
    return "<-- ";
  } else {
    return " -->";
  }
}



static char *left_side_direction(pic_t *pic, char port, int bit)
{
  if (pic_port_direction(pic, port, bit)) {
    return " -->";
  } else {
    return "<-- ";
  }
}



static bool pic_port_value(pic_t *pic, char port, int bit)
{
  switch (port) {
  case 'A':
    return (((pic->r[PIC_REG_PORTA] & ~pic->r[PIC_REG_TRISA]) |
             (pic->in_porta         &  pic->r[PIC_REG_TRISA])) >> bit) & 1;
  case 'B':
    return (((pic->r[PIC_REG_PORTB] & ~pic->r[PIC_REG_TRISB]) |
             (pic->in_portb         &  pic->r[PIC_REG_TRISB])) >> bit) & 1;
  case 'C':
    return (((pic->r[PIC_REG_PORTC] & ~pic->r[PIC_REG_TRISC]) |
             (pic->in_portc         &  pic->r[PIC_REG_TRISC])) >> bit) & 1;
  case 'D':
    return (((pic->r[PIC_REG_PORTD] & ~pic->r[PIC_REG_TRISD]) |
             (pic->in_portd         &  pic->r[PIC_REG_TRISD])) >> bit) & 1;
  case 'E':
    return (((pic->r[PIC_REG_PORTE] & ~pic->r[PIC_REG_TRISE]) |
             (pic->in_porte         &  pic->r[PIC_REG_TRISE])) >> bit) & 1;
  default:
    return false;
  }
}



void chipview_update(pic_t *pic)
{
  mvprintw( 0, 1, "       +------|__|------+");
  mvprintw( 1, 1, "       |             RB7| %s %d",
    right_side_direction(pic, 'B', 7), pic_port_value(pic, 'B', 7));

  mvprintw( 2, 1, "%d %s |RA0          RB6| %s %d",
    pic_port_value(pic, 'A', 0), left_side_direction(pic, 'A', 0),
    right_side_direction(pic, 'B', 6), pic_port_value(pic, 'B', 6));

  mvprintw( 3, 1, "%d %s |RA1          RB5| %s %d",
    pic_port_value(pic, 'A', 1), left_side_direction(pic, 'A', 1),
    right_side_direction(pic, 'B', 5), pic_port_value(pic, 'B', 5));

  mvprintw( 4, 1, "%d %s |RA2          RB4| %s %d",
    pic_port_value(pic, 'A', 2), left_side_direction(pic, 'A', 2),
    right_side_direction(pic, 'B', 4), pic_port_value(pic, 'B', 4));

  mvprintw( 5, 1, "%d %s |RA3          RB3| %s %d",
    pic_port_value(pic, 'A', 3), left_side_direction(pic, 'A', 3),
    right_side_direction(pic, 'B', 3), pic_port_value(pic, 'B', 3));

  mvprintw( 6, 1, "%d %s |RA4          RB2| %s %d",
    pic_port_value(pic, 'A', 4), left_side_direction(pic, 'A', 4),
    right_side_direction(pic, 'B', 2), pic_port_value(pic, 'B', 2));

  mvprintw( 7, 1, "%d %s |RA5          RB1| %s %d",
    pic_port_value(pic, 'A', 5), left_side_direction(pic, 'A', 5),
    right_side_direction(pic, 'B', 1), pic_port_value(pic, 'B', 1));

  mvprintw( 8, 1, "%d %s |RE0          RB0| %s %d",
    pic_port_value(pic, 'E', 0), left_side_direction(pic, 'E', 0),
    right_side_direction(pic, 'B', 0), pic_port_value(pic, 'B', 0));

  mvprintw( 9, 1, "%d %s |RE1             |",
    pic_port_value(pic, 'E', 1), left_side_direction(pic, 'E', 1));

  mvprintw(10, 1, "%d %s |RE2             |",
    pic_port_value(pic, 'E', 2), left_side_direction(pic, 'E', 2));

  mvprintw(11, 1, "       |             RD7| %s %d",
    right_side_direction(pic, 'D', 7), pic_port_value(pic, 'D', 7));

  mvprintw(12, 1, "       |             RD6| %s %d",
    right_side_direction(pic, 'D', 6), pic_port_value(pic, 'D', 6));

  mvprintw(13, 1, "       |             RD5| %s %d",
    right_side_direction(pic, 'D', 5), pic_port_value(pic, 'D', 5));

  mvprintw(14, 1, "       |             RD4| %s %d",
    right_side_direction(pic, 'D', 4), pic_port_value(pic, 'D', 4));

  mvprintw(15, 1, "%d %s |RC0       RX/RC7| %s %d",
    pic_port_value(pic, 'C', 0), left_side_direction(pic, 'C', 0),
    right_side_direction(pic, 'C', 7), pic_port_value(pic, 'C', 7));

  mvprintw(16, 1, "%d %s |RC1       TX/RC6| %s %d",
    pic_port_value(pic, 'C', 1), left_side_direction(pic, 'C', 1),
    right_side_direction(pic, 'C', 6), pic_port_value(pic, 'C', 6));

  mvprintw(17, 1, "%d %s |RC2          RC5| %s %d",
    pic_port_value(pic, 'C', 2), left_side_direction(pic, 'C', 2),
    right_side_direction(pic, 'C', 5), pic_port_value(pic, 'C', 5));

  mvprintw(18, 1, "%d %s |RC3/SCL  SDA/RC4| %s %d",
    pic_port_value(pic, 'C', 3), left_side_direction(pic, 'C', 3),
    right_side_direction(pic, 'C', 4), pic_port_value(pic, 'C', 4));

  mvprintw(19, 1, "%d %s |RD0          RD3| %s %d",
    pic_port_value(pic, 'D', 0), left_side_direction(pic, 'D', 0),
    right_side_direction(pic, 'D', 3), pic_port_value(pic, 'D', 3));

  mvprintw(20, 1, "%d %s |RD1          RD2| %s %d",
    pic_port_value(pic, 'D', 1), left_side_direction(pic, 'D', 1),
    right_side_direction(pic, 'D', 2), pic_port_value(pic, 'D', 2));

  mvprintw(21, 1, "       +----------------+");

  refresh();
}



static void chipview_reg_write(pic_t *pic, uint16_t f)
{
  switch (f) {
  case PIC_REG_PORTA:
  case PIC_REG_PORTB:
  case PIC_REG_PORTC:
  case PIC_REG_PORTD:
  case PIC_REG_PORTE:
  case PIC_REG_TRISA:
  case PIC_REG_TRISB:
  case PIC_REG_TRISC:
  case PIC_REG_TRISD:
  case PIC_REG_TRISE:
    chipview_update(pic);
    break;
  default:
    break;
  }
}



void chipview_pause(void)
{
  endwin();
  timeout(-1);
}



void chipview_resume(void)
{
  timeout(0);
  refresh();
}



void chipview_exit(void)
{
  endwin();
}



void chipview_init(pic_t *pic)
{
  initscr();
  atexit(chipview_exit);
  noecho();
  keypad(stdscr, TRUE);
  timeout(0);

  pic->reg_write_hook = chipview_reg_write;
}



