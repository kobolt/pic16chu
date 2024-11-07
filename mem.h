#ifndef _MEM_H
#define _MEM_H

#include <stdint.h>
#include <stdio.h>

#define MEM_PROGRAM_MAX 0x2000
#define MEM_EEPROM_MAX 0x100

typedef struct mem_s {
  uint16_t program[MEM_PROGRAM_MAX];
  uint8_t eeprom[MEM_EEPROM_MAX];
} mem_t;

void mem_init(mem_t *mem);
int mem_load(mem_t *mem, const char *filename);
void mem_eeprom_dump(mem_t *mem, FILE *fh);

#endif /* _MEM_H */
