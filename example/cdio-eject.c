#include <stdio.h>
#include <string.h>
#include <cdio/cdio.h>

static void usage(char * progname)
  {
  fprintf(stderr, "Usage: %s [-t] <device>\n", progname);
  }

int main(int argc, char ** argv)
  {
  driver_return_code_t err;
  int close_tray = 0;
  const char * device = NULL;
  
  if(argc < 2 || argc > 3)
    {
    usage(argv[0]);
    return -1;
    }

  if((argc == 3) && strcmp(argv[1], "-t"))
    {
    usage(argv[0]);
    return -1;
    }

  if(argc == 2)
    device = argv[1];
  else if(argc == 3)
    {
    close_tray = 1;
    device = argv[2];
    }

  if(close_tray)
    {
    err = cdio_close_tray(device, NULL);
    if(err)
      {
      fprintf(stderr, "Closing tray failed for device %s: %s\n",
              device, cdio_driver_errmsg(err));
      return -1;
      }
    }
  else
    {
    err = cdio_eject_media_drive(device);
    if(err)
      {
      fprintf(stderr, "Ejecting failed for device %s: %s\n",
              device, cdio_driver_errmsg(err));
      return -1;
      }
    }

  return 0;
  
  }
