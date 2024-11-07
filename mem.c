#include "mem.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>



void mem_init(mem_t *mem)
{
  int i;

  for (i = 0; i < MEM_PROGRAM_MAX; i++) {
    mem->program[i] = 0x0000;
  }
  for (i = 0; i < MEM_EEPROM_MAX; i++) {
    mem->eeprom[i] = 0x00;
  }
}



int mem_load(mem_t *mem, const char *filename)
{
  FILE *fh;
  char line[128];
  uint8_t byte_count;
  uint16_t address;
  uint8_t record_type;
  uint8_t data;
  int n;

  fh = fopen(filename, "r");
  if (fh == NULL) {
    return -1;
  }

  while (fgets(line, sizeof(line), fh) != NULL) {
    if (sscanf(line, ":%02hhx%04hx%02hhx",
      &byte_count, &address, &record_type) != 3) {
      continue; /* Not a Intel HEX line. */
    }

    if (record_type != 0) {
      continue; /* Only check data records. */
    }

    address /= 2; /* Convert from 8-bit to 16-bit word addresses. */

    /* NOTE: Checksum is not calculcated nor checked. */

    if (byte_count > 16) {
      continue; /* Byte overflow, ignore. */
    }

    n = 9;
    while (byte_count > 0) {
      sscanf(&line[n], "%02hhx", &data);
      n += 2;
      if (address < MEM_PROGRAM_MAX) {
        mem->program[address] = data;
      } else if (address >= 0x2100 && address <= 0x21FF) {
        mem->eeprom[address - 0x2100] = data;
      }

      sscanf(&line[n], "%02hhx", &data);
      n += 2;
      if (address < MEM_PROGRAM_MAX) {
        mem->program[address] += (data << 8);
      }

      address++;
      byte_count -= 2;
    }
  }

  fclose(fh);
  return 0;
}



void mem_eeprom_dump(mem_t *mem, FILE *fh)
{
  for (int i = 0; i < MEM_EEPROM_MAX; i++) {
    if (i % 16 == 0) {
      fprintf(fh, "%02x: ", i);
    }
    fprintf(stdout, "%02x ", mem->eeprom[i]);
    if (i % 16 == 15) {
      fprintf(fh, "\n");
    }
  }
}



