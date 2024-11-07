#ifndef _PIC_H
#define _PIC_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "mem.h"

#define PIC_STACK_SIZE 8
#define PIC_REGISTER_MAX 0x200

#define PIC_REG_INDF     0x000
#define PIC_REG_PCL      0x002
#define PIC_REG_STATUS   0x003
#define PIC_REG_FSR      0x004
#define PIC_REG_PORTA    0x005
#define PIC_REG_PORTB    0x006
#define PIC_REG_PORTC    0x007
#define PIC_REG_PORTD    0x008
#define PIC_REG_PORTE    0x009
#define PIC_REG_PCLATH   0x00A
#define PIC_REG_PIR1     0x00C
#define PIC_REG_RCREG    0x01A
#define PIC_REG_RCSTA    0x018
#define PIC_REG_TXREG    0x019

#define PIC_REG_INDF_1   0x080
#define PIC_REG_PCL_1    0x082
#define PIC_REG_STATUS_1 0x083
#define PIC_REG_FSR_1    0x084
#define PIC_REG_PCLATH_1 0x08A
#define PIC_REG_TRISA    0x085
#define PIC_REG_TRISB    0x086
#define PIC_REG_TRISC    0x087
#define PIC_REG_TRISD    0x088
#define PIC_REG_TRISE    0x089
#define PIC_REG_TXSTA    0x098

#define PIC_REG_INDF_2   0x100
#define PIC_REG_PCL_2    0x102
#define PIC_REG_STATUS_2 0x103
#define PIC_REG_FSR_2    0x104
#define PIC_REG_PCLATH_2 0x10A
#define PIC_REG_EEDATA   0x10C
#define PIC_REG_EEADR    0x10D

#define PIC_REG_INDF_3   0x180
#define PIC_REG_PCL_3    0x182
#define PIC_REG_STATUS_3 0x183
#define PIC_REG_FSR_3    0x184
#define PIC_REG_PCLATH_3 0x18A
#define PIC_REG_EECON1   0x18C

typedef struct pic_s pic_t;
typedef void (*pic_reg_read_notify_hook_t)(pic_t *, uint16_t);
typedef void (*pic_reg_write_notify_hook_t)(pic_t *, uint16_t);

struct pic_s {
  uint16_t pc;
  uint8_t w;
  uint8_t r[PIC_REGISTER_MAX];
  uint16_t stack[PIC_STACK_SIZE];
  uint8_t sp;
  uint32_t cycle;
  uint8_t in_porta;
  uint8_t in_portb;
  uint8_t in_portc;
  uint8_t in_portd;
  uint8_t in_porte;
  mem_t *mem;
  pic_reg_read_notify_hook_t reg_read_hook;
  pic_reg_write_notify_hook_t reg_write_hook;
};

void pic_trace_init(void);
void pic_trace_dump(FILE *fh);
void pic_port_trace_init(void);
void pic_port_trace_dump(FILE *fh);
void pic_init(pic_t *pic, mem_t *mem);
void pic_reg_dump(pic_t *pic, FILE *fh);
void pic_port_dump(pic_t *pic, FILE *fh);
void pic_execute(pic_t *pic, mem_t *mem);
int16_t pic_uart_tx_read(pic_t *pic);
void pic_uart_rx_write(pic_t *pic, uint8_t data);

#endif /* _PIC_H */
