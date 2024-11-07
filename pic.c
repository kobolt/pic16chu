#include "pic.h"
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mem.h"
#include "panic.h"

#define PIC_STATUS_C   0
#define PIC_STATUS_DC  1
#define PIC_STATUS_Z   2
#define PIC_STATUS_PD  3
#define PIC_STATUS_TO  4
#define PIC_STATUS_RP0 5
#define PIC_STATUS_RP1 6
#define PIC_STATUS_IRP 7

#define PIC_TRACE_BUFFER_SIZE 512
#define PIC_TRACE_BUFFER_ENTRY 80

#define pic_status_set(x, b)   (((pic_t *)x)->r[PIC_REG_STATUS] |=  (1 << b));
#define pic_status_clear(x, b) (((pic_t *)x)->r[PIC_REG_STATUS] &= ~(1 << b));
#define pic_status_get(x, b)  ((((pic_t *)x)->r[PIC_REG_STATUS] >> b) & 1)



static char pic_trace_buffer[PIC_TRACE_BUFFER_SIZE][PIC_TRACE_BUFFER_ENTRY];
static int pic_trace_buffer_index = 0;



static void pic_trace(pic_t *pic, uint16_t opcode, const char *format, ...)
{
  va_list args;
  char buffer[PIC_TRACE_BUFFER_ENTRY + 2];
  int i;
  int n = 0;

  n += snprintf(&buffer[n], PIC_TRACE_BUFFER_ENTRY - n, "%08x  ", pic->cycle);
  n += snprintf(&buffer[n], PIC_TRACE_BUFFER_ENTRY - n, "%04x  ", pic->pc);
  n += snprintf(&buffer[n], PIC_TRACE_BUFFER_ENTRY - n, "%04x  ", opcode);

  for (i = 0; i < pic->sp; i++) {
    buffer[n] = '_';
    n++;
  }

  va_start(args, format);
  n += vsnprintf(&buffer[n], PIC_TRACE_BUFFER_ENTRY - n, format, args);
  va_end(args);

  while (46 - n > 0) {
    buffer[n] = ' ';
    n++;
    if (n >= PIC_TRACE_BUFFER_ENTRY) {
      break;
    }
  }

  n += snprintf(&buffer[n], PIC_TRACE_BUFFER_ENTRY - n, "W=%02x ", pic->w);

  n += snprintf(&buffer[n], PIC_TRACE_BUFFER_ENTRY - n, "RP=%d ",
    (pic->r[PIC_REG_STATUS] >> 5) & 3);

  n += snprintf(&buffer[n], PIC_TRACE_BUFFER_ENTRY - n, "%c%c%c",
    (pic->r[PIC_REG_STATUS] >> 2) & 1 ? 'Z' : '.',
    (pic->r[PIC_REG_STATUS] >> 1) & 1 ? 'D' : '.',
     pic->r[PIC_REG_STATUS]       & 1 ? 'C' : '.');

  snprintf(&buffer[n], PIC_TRACE_BUFFER_ENTRY - n, "\n");

  strncpy(pic_trace_buffer[pic_trace_buffer_index],
    buffer, PIC_TRACE_BUFFER_ENTRY);
  pic_trace_buffer_index++;
  if (pic_trace_buffer_index >= PIC_TRACE_BUFFER_SIZE) {
    pic_trace_buffer_index = 0;
  }
}



void pic_trace_init(void)
{
  int i;

  for (i = 0; i < PIC_TRACE_BUFFER_SIZE; i++) {
    pic_trace_buffer[i][0] = '\0';
  }
  pic_trace_buffer_index = 0;
}



void pic_trace_dump(FILE *fh)
{
  int i;

  for (i = pic_trace_buffer_index; i < PIC_TRACE_BUFFER_SIZE; i++) {
    if (pic_trace_buffer[i][0] != '\0') {
      fprintf(fh, pic_trace_buffer[i]);
    }
  }
  for (i = 0; i < pic_trace_buffer_index; i++) {
    if (pic_trace_buffer[i][0] != '\0') {
      fprintf(fh, pic_trace_buffer[i]);
    }
  }
}



void pic_init(pic_t *pic, mem_t *mem)
{
  memset(pic, 0, sizeof(pic_t));
  pic->mem = mem;
}



void pic_reg_dump(pic_t *pic, FILE *fh)
{
  fprintf(fh, "    ");
  for (int i = 0; i < 16; i++) {
    fprintf(fh, " %x ", i);
  }
  fprintf(fh, "\n");

  for (int i = 0; i < PIC_REGISTER_MAX; i++) {
    if (i % 16 == 0) {
      fprintf(fh, "%02x: ", i / 16);
    }
    fprintf(fh, "%02x ", pic->r[i]);
    if (i % 16 == 15) {
      fprintf(fh, "\n");
    }
  }
}



static void pic_port_dump_detail(FILE *fh, uint8_t direction, uint8_t value)
{
  for (int i = 0; i < 8; i++) {
    fprintf(fh, "  %d %c--%c %d\n", i,
      ((direction >> i) & 1) ? '<' : ' ',
      ((direction >> i) & 1) ? ' ' : '>',
      (value >> i) & 1);
  }
}



void pic_port_dump(pic_t *pic, FILE *fh)
{
  fprintf(fh, "PORTA = 0x%02x, TRISA = 0x%02x, Input = %02x\n",
    pic->r[PIC_REG_PORTA], pic->r[PIC_REG_TRISA], pic->in_porta);
  pic_port_dump_detail(fh, pic->r[PIC_REG_TRISA],
    (pic->r[PIC_REG_PORTA] & ~pic->r[PIC_REG_TRISA]) |
    (pic->in_porta         &  pic->r[PIC_REG_TRISA]));

  fprintf(fh, "PORTB = 0x%02x, TRISB = 0x%02x, Input = %02x\n",
    pic->r[PIC_REG_PORTB], pic->r[PIC_REG_TRISB], pic->in_portb);
  pic_port_dump_detail(fh, pic->r[PIC_REG_TRISB],
    (pic->r[PIC_REG_PORTB] & ~pic->r[PIC_REG_TRISB]) |
    (pic->in_portb         &  pic->r[PIC_REG_TRISB]));

  fprintf(fh, "PORTC = 0x%02x, TRISC = 0x%02x, Input = %02x\n",
    pic->r[PIC_REG_PORTC], pic->r[PIC_REG_TRISC], pic->in_portc);
  pic_port_dump_detail(fh, pic->r[PIC_REG_TRISC],
    (pic->r[PIC_REG_PORTC] & ~pic->r[PIC_REG_TRISC]) |
    (pic->in_portc         &  pic->r[PIC_REG_TRISC]));

  fprintf(fh, "PORTD = 0x%02x, TRISD = 0x%02x, Input = %02x\n",
    pic->r[PIC_REG_PORTD], pic->r[PIC_REG_TRISD], pic->in_portd);
  pic_port_dump_detail(fh, pic->r[PIC_REG_TRISD],
    (pic->r[PIC_REG_PORTD] & ~pic->r[PIC_REG_TRISD]) |
    (pic->in_portd         &  pic->r[PIC_REG_TRISD]));

  fprintf(fh, "PORTE = 0x%02x, TRISE = 0x%02x, Input = %02x\n",
    pic->r[PIC_REG_PORTE], pic->r[PIC_REG_TRISE], pic->in_porte);
  pic_port_dump_detail(fh, pic->r[PIC_REG_TRISE],
    (pic->r[PIC_REG_PORTE] & ~pic->r[PIC_REG_TRISE]) |
    (pic->in_porte         &  pic->r[PIC_REG_TRISE]));
}



static inline void pic_flag_z(pic_t *pic, uint8_t value)
{
  if (value == 0) {
    pic_status_set(pic, PIC_STATUS_Z);
  } else {
    pic_status_clear(pic, PIC_STATUS_Z);
  }
}



static inline void pic_flag_add(pic_t *pic, uint8_t value)
{
  /* NOTE: DC status flag is not handled! */
  if (value + pic->w > 0xFF) {
    pic_status_set(pic, PIC_STATUS_C);
  } else {
    pic_status_clear(pic, PIC_STATUS_C);
  }
}



static inline void pic_flag_sub(pic_t *pic, uint8_t value)
{
  /* NOTE: DC status flag is not handled! */
  if (value - pic->w < 0) {
    pic_status_clear(pic, PIC_STATUS_C);
  } else {
    pic_status_set(pic, PIC_STATUS_C);
  }
}



static uint8_t pic_reg_read(pic_t *pic, uint16_t f)
{
  f |= (pic_status_get(pic, PIC_STATUS_RP0) << 7);
  f |= (pic_status_get(pic, PIC_STATUS_RP1) << 8);
  switch (f) {
  case PIC_REG_INDF:
  case PIC_REG_INDF_1:
  case PIC_REG_INDF_2:
  case PIC_REG_INDF_3:
    f  =  pic->r[PIC_REG_FSR];
    f |= (pic_status_get(pic, PIC_STATUS_IRP) << 8);
    break;
  case PIC_REG_PCL:
  case PIC_REG_PCL_1:
  case PIC_REG_PCL_2:
  case PIC_REG_PCL_3:
    return pic->pc & 0xFF;
  case PIC_REG_STATUS_1:
  case PIC_REG_STATUS_2:
  case PIC_REG_STATUS_3:
    f = PIC_REG_STATUS;
    break;
  case PIC_REG_FSR_1:
  case PIC_REG_FSR_2:
  case PIC_REG_FSR_3:
    f = PIC_REG_FSR;
    break;
  case PIC_REG_PCLATH_1:
  case PIC_REG_PCLATH_2:
  case PIC_REG_PCLATH_3:
    f = PIC_REG_PCLATH;
    break;
  case PIC_REG_RCREG:
    pic->r[PIC_REG_PIR1] &= ~0x20; /* Clear RCIF once RCREG has been read. */
    break;
  case PIC_REG_PIR1:
    pic->r[PIC_REG_PIR1] |= 0x10; /* Make sure TXIF is always set. */
    break;
  case PIC_REG_TXSTA:
    pic->r[PIC_REG_TXSTA] |= 0x02; /* Make sure TRMT is always set. */
    break;
  case PIC_REG_PORTA:
    return (pic->r[PIC_REG_PORTA] & ~pic->r[PIC_REG_TRISA]) |
           (pic->in_porta         &  pic->r[PIC_REG_TRISA]);
  case PIC_REG_PORTB:
    return (pic->r[PIC_REG_PORTB] & ~pic->r[PIC_REG_TRISB]) |
           (pic->in_portb         &  pic->r[PIC_REG_TRISB]);
  case PIC_REG_PORTC:
    return (pic->r[PIC_REG_PORTC] & ~pic->r[PIC_REG_TRISC]) |
           (pic->in_portc         &  pic->r[PIC_REG_TRISC]);
  case PIC_REG_PORTD:
    return (pic->r[PIC_REG_PORTD] & ~pic->r[PIC_REG_TRISD]) |
           (pic->in_portd         &  pic->r[PIC_REG_TRISD]);
  case PIC_REG_PORTE:
    return (pic->r[PIC_REG_PORTE] & ~pic->r[PIC_REG_TRISE]) |
           (pic->in_porte         &  pic->r[PIC_REG_TRISE]);
  default:
    break;
  }

  if (pic->reg_read_hook != NULL) {
    (pic->reg_read_hook)(pic, f);
  }

  return pic->r[f];
}



static void pic_reg_write(pic_t *pic, uint16_t f, uint8_t value)
{
  f |= (pic_status_get(pic, PIC_STATUS_RP0) << 7);
  f |= (pic_status_get(pic, PIC_STATUS_RP1) << 8);
  switch (f) {
  case PIC_REG_INDF:
  case PIC_REG_INDF_1:
  case PIC_REG_INDF_2:
  case PIC_REG_INDF_3:
    f  =  pic->r[PIC_REG_FSR];
    f |= (pic_status_get(pic, PIC_STATUS_IRP) << 8);
    break;
  case PIC_REG_PCL:
  case PIC_REG_PCL_1:
  case PIC_REG_PCL_2:
  case PIC_REG_PCL_3:
    pic->pc = (pic->pc & 0xFF00) | value;
    break;
  case PIC_REG_STATUS_1:
  case PIC_REG_STATUS_2:
  case PIC_REG_STATUS_3:
    f = PIC_REG_STATUS;
    break;
  case PIC_REG_FSR_1:
  case PIC_REG_FSR_2:
  case PIC_REG_FSR_3:
    f = PIC_REG_FSR;
    break;
  case PIC_REG_PCLATH_1:
  case PIC_REG_PCLATH_2:
  case PIC_REG_PCLATH_3:
    f = PIC_REG_PCLATH;
    break;
  case PIC_REG_RCSTA:
    if ((value & 0x10) == 0) {
      pic->r[PIC_REG_RCSTA] &= ~0x02; /* Clear OERR when CREN is cleared. */
    }
    break;
  case PIC_REG_EECON1:
    if (value & 0x01) {
      if ((value & 0x80) == 0) {
        /* Read from data memory EEPROM. */
        pic->r[PIC_REG_EEDATA] = pic->mem->eeprom[pic->r[PIC_REG_EEADR]];
      } else {
        panic("Reading from program memory not implemented!\n");
      }
    } else if (value & 0x02) {
      if ((value & 0x80) == 0) {
        /* Write to data memory EEPROM. */
        pic->mem->eeprom[pic->r[PIC_REG_EEADR]] = pic->r[PIC_REG_EEDATA];
        value &= ~0x02; /* Clear WR again to indicate write done already. */
      } else {
        panic("Writing to program memory not implemented!\n");
      }
    }
    break;
  default:
    break;
  }

  pic->r[f] = value;

  if (pic->reg_write_hook != NULL) {
    (pic->reg_write_hook)(pic, f);
  }
}



void pic_execute(pic_t *pic, mem_t *mem)
{
  uint16_t opcode;
  uint16_t k;
  uint8_t b;
  uint8_t f;
  bool d;
  bool bit;

  opcode = mem->program[pic->pc & 0x1FFF];

  if ((opcode & 0xFF9F) == 0x0000) { /* NOP */
    pic_trace(pic, opcode, "NOP");
    pic->pc++;
    pic->cycle++;

  } else if ((opcode & 0xFE00) == 0x3E00) { /* ADDLW */
    k = opcode & 0xFF;
    pic_trace(pic, opcode, "ADDLW 0x%02x", k);
    pic_flag_add(pic, k);
    pic->w += k;
    pic_flag_z(pic, pic->w);
    pic->pc++;
    pic->cycle++;
#ifdef PANIC_ON_3FFF
    if (opcode == 0x3FFF) {
      panic("Suspicious 0x3FFF opcode!\n");
    }
#endif

  } else if ((opcode & 0xFF00) == 0x700) { /* ADDWF */
    f = opcode & 0x7F;
    d = (opcode >> 7) & 1;
    pic_trace(pic, opcode, "ADDWF 0x%02x, %d", f, d);
    if (d) {
      pic_flag_add(pic, pic_reg_read(pic, f));
      pic_reg_write(pic, f, pic_reg_read(pic, f) + pic->w);
      pic_flag_z(pic, pic_reg_read(pic, f));
    } else {
      pic_flag_add(pic, pic_reg_read(pic, f));
      pic->w = pic_reg_read(pic, f) + pic->w;
      pic_flag_z(pic, pic->w);
    }
    pic->pc++;
    pic->cycle++;

  } else if ((opcode & 0xFF00) == 0x3900) { /* ANDLW */
    k = opcode & 0xFF;
    pic_trace(pic, opcode, "ANDLW 0x%02x", k);
    pic->w &= k;
    pic_flag_z(pic, pic->w);
    pic->pc++;
    pic->cycle++;

  } else if ((opcode & 0xFF00) == 0x500) { /* ANDWF */
    f = opcode & 0x7F;
    d = (opcode >> 7) & 1;
    pic_trace(pic, opcode, "ANDWF 0x%02x, %d", f, d);
    if (d) {
      pic_reg_write(pic, f, pic_reg_read(pic, f) & pic->w);
      pic_flag_z(pic, pic_reg_read(pic, f));
    } else {
      pic->w = pic_reg_read(pic, f) & pic->w;
      pic_flag_z(pic, pic->w);
    }
    pic->pc++;
    pic->cycle++;

  } else if ((opcode & 0xFC00) == 0x1000) { /* BCF */
    f = opcode & 0x7F;
    b = (opcode >> 7) & 0x7;
    pic_trace(pic, opcode, "BCF 0x%02x, %d", f, b);
    pic_reg_write(pic, f, pic_reg_read(pic, f) & ~(1 << b));
    pic->pc++;
    pic->cycle++;

  } else if ((opcode & 0xFC00) == 0x1400) { /* BSF */
    f = opcode & 0x7F;
    b = (opcode >> 7) & 0x7;
    pic_trace(pic, opcode, "BSF 0x%02x, %d", f, b);
    pic_reg_write(pic, f, pic_reg_read(pic, f) | (1 << b));
    pic->pc++;
    pic->cycle++;

  } else if ((opcode & 0xFC00) == 0x1800) { /* BTFSC */
    f = opcode & 0x7F;
    b = (opcode >> 7) & 0x7;
    pic_trace(pic, opcode, "BTFSC 0x%02x, %d", f, b);
    if ((pic_reg_read(pic, f) >> b) & 1) {
      pic->pc++;
      pic->cycle++;
    } else {
      pic->pc += 2;
      pic->cycle += 2;
    }

  } else if ((opcode & 0xFC00) == 0x1C00) { /* BTFSS */
    f = opcode & 0x7F;
    b = (opcode >> 7) & 0x7;
    pic_trace(pic, opcode, "BTFSS 0x%02x, %d", f, b);
    if ((pic_reg_read(pic, f) >> b) & 1) {
      pic->pc += 2;
      pic->cycle += 2;
    } else {
      pic->pc++;
      pic->cycle++;
    }

  } else if ((opcode & 0xF800) == 0x2000) { /* CALL */
    k = opcode & 0x7FF;
    pic_trace(pic, opcode, "CALL 0x%04x", k);
    if (pic->sp == PIC_STACK_SIZE) {
      panic("Stack overflow on call!\n");
    } else {
      pic->stack[pic->sp] = pic->pc + 1;
      pic->sp++;
      pic->pc = k;
      pic->pc += (((pic->r[PIC_REG_PCLATH] >> 3) & 0x3) << 11);
      pic->cycle += 2;
    }

  } else if ((opcode & 0xFF80) == 0x180) { /* CLRF */
    f = opcode & 0x7F;
    pic_trace(pic, opcode, "CLRF 0x%02x", f);
    pic_reg_write(pic, f, 0);
    pic_flag_z(pic, 0);
    pic->pc++;
    pic->cycle++;

  } else if ((opcode & 0xFF80) == 0x100) { /* CLRW */
    pic_trace(pic, opcode, "CLRW");
    pic->w = 0;
    pic_flag_z(pic, 0);
    pic->pc++;
    pic->cycle++;

  } else if ((opcode & 0xFF00) == 0x900) { /* COMF */
    f = opcode & 0x7F;
    d = (opcode >> 7) & 1;
    pic_trace(pic, opcode, "COMF 0x%02x, %d", f, d);
    if (d) {
      pic_reg_write(pic, f, ~pic_reg_read(pic, f));
      pic_flag_z(pic, pic_reg_read(pic, f));
    } else {
      pic->w = ~pic_reg_read(pic, f);
      pic_flag_z(pic, pic->w);
    }
    pic->pc++;
    pic->cycle++;

  } else if ((opcode & 0xFF00) == 0x300) { /* DECF */
    f = opcode & 0x7F;
    d = (opcode >> 7) & 1;
    pic_trace(pic, opcode, "DECF 0x%02x, %d", f, d);
    if (d) {
      pic_reg_write(pic, f, pic_reg_read(pic, f) - 1);
      pic_flag_z(pic, pic_reg_read(pic, f));
    } else {
      pic->w = pic_reg_read(pic, f) - 1;
      pic_flag_z(pic, pic->w);
    }
    pic->pc++;
    pic->cycle++;

  } else if ((opcode & 0xFF00) == 0xB00) { /* DECFSZ */
    f = opcode & 0x7F;
    d = (opcode >> 7) & 1;
    pic_trace(pic, opcode, "DECFSZ 0x%02x, %d", f, d);
    if (d) {
      pic_reg_write(pic, f, pic_reg_read(pic, f) - 1);
      if (pic_reg_read(pic, f)) {
        pic->pc++;
        pic->cycle++;
      } else {
        pic->pc += 2;
        pic->cycle += 2;
      }
    } else {
      pic->w = pic_reg_read(pic, f) - 1;
      if (pic->w) {
        pic->pc++;
        pic->cycle++;
      } else {
        pic->pc += 2;
        pic->cycle += 2;
      }
    }

  } else if ((opcode & 0xF800) == 0x2800) { /* GOTO */
    k = opcode & 0x7FF;
    pic_trace(pic, opcode, "GOTO 0x%04x", k);
    pic->pc = k;
    pic->pc += (((pic->r[PIC_REG_PCLATH] >> 3) & 0x3) << 11);
    pic->cycle += 2;

  } else if ((opcode & 0xFF00) == 0x3800) { /* IORLW */
    k = opcode & 0xFF;
    pic_trace(pic, opcode, "IORLW 0x%02x", k);
    pic->w |= k;
    pic_flag_z(pic, pic->w);
    pic->pc++;
    pic->cycle++;

  } else if ((opcode & 0xFF00) == 0x400) { /* IORWF */
    f = opcode & 0x7F;
    d = (opcode >> 7) & 1;
    pic_trace(pic, opcode, "IORWF 0x%02x, %d", f, d);
    if (d) {
      pic_reg_write(pic, f, pic_reg_read(pic, f) | pic->w);
      pic_flag_z(pic, pic_reg_read(pic, f));
    } else {
      pic->w = pic_reg_read(pic, f) | pic->w;
      pic_flag_z(pic, pic->w);
    }
    pic->pc++;
    pic->cycle++;

  } else if ((opcode & 0xFF00) == 0xA00) { /* INCF */
    f = opcode & 0x7F;
    d = (opcode >> 7) & 1;
    pic_trace(pic, opcode, "INCF 0x%02x, %d", f, d);
    if (d) {
      pic_reg_write(pic, f, pic_reg_read(pic, f) + 1);
      pic_flag_z(pic, pic_reg_read(pic, f));
    } else {
      pic->w = pic_reg_read(pic, f) + 1;
      pic_flag_z(pic, pic->w);
    }
    pic->pc++;
    pic->cycle++;

  } else if ((opcode & 0xFF00) == 0xF00) { /* INCFSZ */
    f = opcode & 0x7F;
    d = (opcode >> 7) & 1;
    pic_trace(pic, opcode, "INCFSZ 0x%02x, %d", f, d);
    if (d) {
      pic_reg_write(pic, f, pic_reg_read(pic, f) + 1);
      if (pic_reg_read(pic, f)) {
        pic->pc++;
        pic->cycle++;
      } else {
        pic->pc += 2;
        pic->cycle += 2;
      }
    } else {
      pic->w = pic_reg_read(pic, f) + 1;
      if (pic->w) {
        pic->pc++;
        pic->cycle++;
      } else {
        pic->pc += 2;
        pic->cycle += 2;
      }
    }

  } else if ((opcode & 0xFF00) == 0x800) { /* MOVF */
    f = opcode & 0x7F;
    d = (opcode >> 7) & 1;
    pic_trace(pic, opcode, "MOVF 0x%02x, %d", f, d);
    if (d) {
      pic_reg_write(pic, f, pic->w);
      pic_flag_z(pic, pic_reg_read(pic, f));
    } else {
      pic->w = pic_reg_read(pic, f);
      pic_flag_z(pic, pic->w);
    }
    pic->pc++;
    pic->cycle++;

  } else if ((opcode & 0xFC00) == 0x3000) { /* MOVLW */
    k = opcode & 0xFF;
    pic_trace(pic, opcode, "MOVLW 0x%02x", k);
    pic->w = k;
    pic->pc++;
    pic->cycle++;

  } else if ((opcode & 0xFF80) == 0x80) { /* MOVWF */
    f = opcode & 0x7F;
    pic_trace(pic, opcode, "MOVWF 0x%02x", f);
    pic_reg_write(pic, f, pic->w);
    pic->pc++;
    pic->cycle++;

  } else if ((opcode & 0xFC00) == 0x3400) { /* RETLW */
    k = opcode & 0xFF;
    pic_trace(pic, opcode, "RETLW 0x%02x", k);
    pic->w = k;
    if (pic->sp == 0) {
      panic("Attempted to return with no stack!\n");
    } else {
      pic->sp--;
      pic->pc = pic->stack[pic->sp];
      pic->cycle += 2;
    }

  } else if (opcode == 0x8) { /* RETURN */
    pic_trace(pic, opcode, "RETURN");
    if (pic->sp == 0) {
      panic("Attempted to return with no stack!\n");
    } else {
      pic->sp--;
      pic->pc = pic->stack[pic->sp];
      pic->cycle += 2;
    }

  } else if ((opcode & 0xFF00) == 0xD00) { /* RLF */
    f = opcode & 0x7F;
    d = (opcode >> 7) & 1;
    pic_trace(pic, opcode, "RLF 0x%02x, %d", f, d);
    bit = pic_reg_read(pic, f) & 0x80;
    if (d) {
      pic_reg_write(pic, f, pic_reg_read(pic, f) << 1);
      if (pic_status_get(pic, PIC_STATUS_C)) {
        pic_reg_write(pic, f, pic_reg_read(pic, f) | 1);
      }
    } else {
      pic->w = pic_reg_read(pic, f) << 1;
      if (pic_status_get(pic, PIC_STATUS_C)) {
        pic->w |= 1;
      }
    }
    if (bit) {
      pic_status_set(pic, PIC_STATUS_C);
    } else {
      pic_status_clear(pic, PIC_STATUS_C);
    }
    pic->pc++;
    pic->cycle++;

  } else if ((opcode & 0xFF00) == 0xC00) { /* RRF */
    f = opcode & 0x7F;
    d = (opcode >> 7) & 1;
    pic_trace(pic, opcode, "RRF 0x%02x, %d", f, d);
    bit = pic_reg_read(pic, f) & 1;
    if (d) {
      pic_reg_write(pic, f, pic_reg_read(pic, f) >> 1);
      if (pic_status_get(pic, PIC_STATUS_C)) {
        pic_reg_write(pic, f, pic_reg_read(pic, f) | 0x80);
      }
    } else {
      pic->w = pic_reg_read(pic, f) >> 1;
      if (pic_status_get(pic, PIC_STATUS_C)) {
        pic->w |= 0x80;
      }
    }
    if (bit) {
      pic_status_set(pic, PIC_STATUS_C);
    } else {
      pic_status_clear(pic, PIC_STATUS_C);
    }
    pic->pc++;
    pic->cycle++;

  } else if ((opcode & 0xFE00) == 0x3C00) { /* SUBLW */
    k = opcode & 0xFF;
    pic_trace(pic, opcode, "SUBLW 0x%02x", k);
    pic_flag_sub(pic, k);
    pic->w = k - pic->w;
    pic_flag_z(pic, pic->w);
    pic->pc++;
    pic->cycle++;

  } else if ((opcode & 0xFF00) == 0x200) { /* SUBWF */
    f = opcode & 0x7F;
    d = (opcode >> 7) & 1;
    pic_trace(pic, opcode, "SUBWF 0x%02x, %d", f, d);
    if (d) {
      pic_flag_sub(pic, pic_reg_read(pic, f));
      pic_reg_write(pic, f, pic_reg_read(pic, f) - pic->w);
      pic_flag_z(pic, pic_reg_read(pic, f));
    } else {
      pic_flag_sub(pic, pic_reg_read(pic, f));
      pic->w = pic_reg_read(pic, f) - pic->w;
      pic_flag_z(pic, pic->w);
    }
    pic->pc++;
    pic->cycle++;

  } else if ((opcode & 0xFF00) == 0xE00) { /* SWAPF */
    f = opcode & 0x7F;
    d = (opcode >> 7) & 1;
    pic_trace(pic, opcode, "SWAPF 0x%02x, %d", f, d);
    if (d) {
      pic_reg_write(pic, f, (pic_reg_read(pic, f) >> 4) |
                           ((pic_reg_read(pic, f) << 4) & 0xF0));
    } else {
      pic->w = (pic_reg_read(pic, f) >> 4) |
              ((pic_reg_read(pic, f) << 4) & 0xF0);
    }
    pic->pc++;
    pic->cycle++;

  } else if ((opcode & 0xFFFC) == 0x64) { /* TRIS */
    f = opcode & 0x3;
    pic_trace(pic, opcode, "TRIS %d", f);
    if (f == 1) {
      pic->r[PIC_REG_TRISA] = pic->w;
    } else if (f == 2) {
      pic->r[PIC_REG_TRISB] = pic->w;
    } else if (f == 3) {
      pic->r[PIC_REG_TRISC] = pic->w;
    }
    pic->pc++;
    pic->cycle++;

  } else if ((opcode & 0xFF00) == 0x3A00) { /* XORLW */
    k = opcode & 0xFF;
    pic_trace(pic, opcode, "XORLW 0x%02x", k);
    pic->w ^= k;
    pic_flag_z(pic, pic->w);
    pic->pc++;
    pic->cycle++;

  } else if ((opcode & 0xFF00) == 0x600) { /* XORWF */
    f = opcode & 0x7F;
    d = (opcode >> 7) & 1;
    pic_trace(pic, opcode, "XORWF 0x%02x, %d", f, d);
    if (d) {
      pic_reg_write(pic, f, pic_reg_read(pic, f) ^ pic->w);
      pic_flag_z(pic, pic_reg_read(pic, f));
    } else {
      pic->w = pic_reg_read(pic, f) ^ pic->w;
      pic_flag_z(pic, pic->w);
    }
    pic->pc++;
    pic->cycle++;

  } else {
    panic("Unhandled opcode: 0x%04x @ 0x%04x\n", opcode, pic->pc);
  }
}



