#include "cdp1802.h"
#include "display.h"

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>

int serial_connection;

void outp(int port, uint8_t data)
{
   if (port == 0x02)
   {
      instruction_register(data);
   }
   else if (port == 0x03)
   {
      character_register(data);
      printf("character '%c'\n", data);
   }
}

int serial = -1;
int open_port(void)
{
   int fd = open("/dev/ttyACM0", O_RDWR | O_NOCTTY | O_NDELAY);

   if (fd == -1)
   {
      perror("open_port: Unable to open /dev/ttyf1 - ");
   }
   else
   {
      fcntl(fd, F_SETFL, 0);
   }

   return fd;
}

uint8_t serial_port_keys(int fd)
{
   static uint8_t last_keys = 0xff;

   char ac[8] = { };

   write(fd, "\r", 1);

   if (read(fd, ac, sizeof(ac)) != sizeof ac)
   {
      return last_keys;
   }
   else if (strlen(ac) >= 6)
   {
      uint8_t scan = ~0x00;

      ac[6] = 0;

      scan &= ac[5] == '0' ? ~0x01 : 0xff;
      scan &= ac[4] == '0' ? ~0x02 : 0xff;
      scan &= ac[3] == '0' ? ~0x08 : 0xff;
      scan &= ac[2] == '0' ? ~0x10 : 0xff;
      scan &= ac[1] == '0' ? ~0x20 : 0xff;
      scan &= ac[0] == '0' ? ~0x40 : 0xff;

      last_keys = scan;
      return scan;
   }
   else
   {
      return last_keys;
   }
}

int stop;

void handler(int signal)
{
   if (signal == SIGINT)
   {
      stop = 1;
   }
}

uint8_t keys[0x100];
uint32_t key_total;
uint32_t key_index;
uint32_t dwell;

uint8_t key_to_code(uint8_t key)
{
   uint8_t scan = 0xff;

   scan &= key & 1 << 5 ? ~0x01 : 0xff;   // probably the command button
   scan &= key & 1 << 4 ? ~0x02 : 0xff;   // space
   scan &= key & 1 << 3 ? ~0x08 : 0xff;   // e
   scan &= key & 1 << 2 ? ~0x10 : 0xff;   // o
   scan &= key & 1 << 1 ? ~0x20 : 0xff;   // s
   scan &= key & 1 << 0 ? ~0x40 : 0xff;   // u

   return scan;
}

int inp(int addr)
{
   uint8_t data = 0;
   if (addr == 4)
   {
      if (serial_connection)
      {
         data = serial_port_keys(serial);
      }
      else
      {
         data = key_to_code(keys[key_index]);

         printf("@0x%02x\n", keys[key_index]);

         if (dwell < 2)
         {
            ++dwell;
         }
         else
         {
            dwell = 0;
            ++key_index;

            if (key_index >= key_total)
            {
               stop = 1;
            }
         }
      }
   }

   return data;
}

void interrupt()
{
   if (IE == 0)
   {
      return;
   }
   IE = 1;
   T = P | (X << 4);
   X = 2;
   P = 1;
}

int main(int argc, char *argv[])
{
   int opt = -1;
   int rom = -1;
   int in = -1;
   int out = -1;

   while ((opt = getopt(argc, argv, "sr:i:o:")) != -1)
   {
      switch (opt)
      {
         case 's':
            serial = open_port();
            serial_connection = 1;
            break;
         case 'r':
            rom = open(optarg, 0);
            if (rom < 0)
            {
               assert(0);
            }
            break;
         case 'i':
            break;
         case 'o':
            out = open(optarg, 1);
            break;
         default:
            exit(EXIT_FAILURE);
      }
   }

   argv += optind;
   argc -= optind;

   while (argc > 0)
   {
      printf("%s\n", *argv);
      keys[key_total++] = 0;
      keys[key_total++] = strtoul(*argv, 0, 0);
      ++argv;
      --argc;
   }

   keys[key_total++] = 0;

   if (signal(SIGINT, handler) == SIG_ERR)
   {
      perror("signal sigint install handler fail");
   }

   read(rom, M, sizeof M);

   M[0xffff] = 0xff;

   printf("start\n");

   EF1 = 1;
// EF3 = 1;

   while (!stop)
   {
      int idle = !step();

      if (idle)
      {
         interrupt();
      }
   }

   if (out > 0)
   {
      write(out, M, sizeof M);
      close(out);
   }
}
