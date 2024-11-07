#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "pic.h"
#include "mem.h"
#include "chipview.h"
#include "aegl.h"

static pic_t pic;
static mem_t mem;

static int32_t debugger_breakpoint = -1;
static bool debugger_break = false;
static char panic_msg[80];



void panic(const char *format, ...)
{
  va_list args;

  va_start(args, format);
  vsnprintf(panic_msg, sizeof(panic_msg), format, args);
  va_end(args);

  debugger_break = true;
}



static void sig_handler(int sig)
{
  switch (sig) {
  case SIGINT:
    debugger_break = true;
    return;
  }
}



static bool debugger(void)
{
  char cmd[16];
  int value;

  fprintf(stdout, "\n");
  while (1) {
    fprintf(stdout, "%08x:%04x> ", pic.cycle, pic.pc);

    if (fgets(cmd, sizeof(cmd), stdin) == NULL) {
      if (feof(stdin)) {
        exit(EXIT_SUCCESS);
      }
      continue;
    }

    switch (cmd[0]) {
    case '?':
    case 'h':
      fprintf(stdout, "Commands:\n");
      fprintf(stdout, "  q        - Quit\n");
      fprintf(stdout, "  h        - Help\n");
      fprintf(stdout, "  c        - Continue\n");
      fprintf(stdout, "  s        - Step\n");
      fprintf(stdout, "  b <addr> - Breakpoint\n");
      fprintf(stdout, "  t        - Dump PIC Trace\n");
      fprintf(stdout, "  r        - Dump PIC Registers\n");
      fprintf(stdout, "  p        - Dump PIC Ports\n");
      fprintf(stdout, "  e        - Dump PIC EEPROM\n");
      fprintf(stdout, "  A <hex>  - Set input on port A\n");
      fprintf(stdout, "  B <hex>  - Set input on port B\n");
      fprintf(stdout, "  C <hex>  - Set input on port C\n");
      fprintf(stdout, "  D <hex>  - Set input on port D\n");
      fprintf(stdout, "  E <hex>  - Set input on port E\n");
      break;

    case 'c':
      return false;

    case 's':
      return true;

    case 'q':
      exit(EXIT_SUCCESS);
      break;

    case 'b':
      if (sscanf(&cmd[1], "%4x", &value) == 1) {
        debugger_breakpoint = value & 0x1FFF;
        fprintf(stdout, "Breakpoint set: 0x%04x\n", debugger_breakpoint);
      } else {
        if (debugger_breakpoint != -1) {
          fprintf(stdout, "Breakpoint removed: 0x%04x\n", debugger_breakpoint);
        }
        debugger_breakpoint = -1;
      }
      break;

    case 't':
      pic_trace_dump(stdout);
      break;

    case 'r':
      pic_reg_dump(&pic, stdout);
      break;

    case 'p':
      pic_port_dump(&pic, stdout);
      break;

    case 'e':
      mem_eeprom_dump(&mem, stdout);
      break;

    case 'A':
      if (sscanf(&cmd[1], "%2x", &value) == 1) {
        pic.in_porta = value;
        fprintf(stdout, "Port A input set to 0x%02x\n", value);
      }
      break;

    case 'B':
      if (sscanf(&cmd[1], "%2x", &value) == 1) {
        pic.in_portb = value;
        fprintf(stdout, "Port B input set to 0x%02x\n", value);
      }
      break;

    case 'C':
      if (sscanf(&cmd[1], "%2x", &value) == 1) {
        pic.in_portc = value;
        fprintf(stdout, "Port C input set to 0x%02x\n", value);
      }
      break;

    case 'D':
      if (sscanf(&cmd[1], "%2x", &value) == 1) {
        pic.in_portd = value;
        fprintf(stdout, "Port D input set to 0x%02x\n", value);
      }
      break;

    case 'E':
      if (sscanf(&cmd[1], "%2x", &value) == 1) {
        pic.in_porte = value;
        fprintf(stdout, "Port E input set to 0x%02x\n", value);
      }
      break;

    default:
      continue;
    }
  }
}



static void display_help(const char *progname)
{
  fprintf(stdout, "Usage: %s <options> [hex-file]\n", progname);
  fprintf(stdout, "Options:\n"
    "  -h        Display this help.\n"
    "  -d        Break into debugger on start.\n"
    "  -a        AE-GraphicLCD trace and command mode.\n"
    "\n");
  fprintf(stdout,
    "HEX file should be in Intel format with PIC program and EEPROM data.\n"
    "\n");
}



int main(int argc, char *argv[])
{
  int c;
  char *hex_filename = NULL;
  bool aegl_mode = false;

  panic_msg[0] = '\0';
  signal(SIGINT, sig_handler);

  while ((c = getopt(argc, argv, "hda")) != -1) {
    switch (c) {
    case 'h':
      display_help(argv[0]);
      return EXIT_SUCCESS;

    case 'd':
      debugger_break = true;
      break;

    case 'a':
      aegl_mode = true;
      break;

    case '?':
    default:
      display_help(argv[0]);
      return EXIT_FAILURE;
    }
  }

  mem_init(&mem);
  pic_trace_init();
  pic_init(&pic, &mem);

  if (argc <= optind) {
    display_help(argv[0]);
    return EXIT_FAILURE;
  } else {
    hex_filename = argv[optind];
  }

  if (mem_load(&mem, hex_filename) != 0) {
    fprintf(stderr, "Unable to load HEX file: %s\n", hex_filename);
    return EXIT_FAILURE;
  }

  if (aegl_mode) {
    aegl_init(&pic);
  } else {
    chipview_init(&pic);
    chipview_update(&pic);
  }

  while (1) {
    pic_execute(&pic, &mem);

    if (pic.pc == debugger_breakpoint) {
      strncpy(panic_msg, "Break\n", sizeof(panic_msg));
      debugger_break = true;
    }

    if (debugger_break) {
      chipview_pause();
      if (panic_msg[0] != '\0') {
        fprintf(stdout, "%s", panic_msg);
        panic_msg[0] = '\0';
      }
      debugger_break = debugger();
      if (! debugger_break) {
        chipview_resume();
      }
    }
  }

  return EXIT_SUCCESS;
}



