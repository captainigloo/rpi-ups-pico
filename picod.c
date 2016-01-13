/**\file
 * \brief UPS PIco control daemon.
 *
 * \copyright
 * This programme is released as open source, under the terms of an MIT/X style
 * licence. See the accompanying LICENSE file for details.
 *
 * \see Source Code Repository: https://github.com/ef-gy/rpi-ups-pico
 * \see Licence Terms: https://github.com/ef-gy/rpi-ups-pico/blob/master/LICENSE
 *
 * \todo Implement the FSSD feature. This ought to be trivial.
 */

/* for open() */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

/* for usleep(), write() */
#include <unistd.h>

/* for snprintf() */
#include <stdio.h>

/* for errno */
#include <errno.h>

#define MAX_GPIO_FN 256
#define MAX_BUFFER 32

static int export(int gpio) {
  int fd;
  char bf[MAX_BUFFER];
  int rv = 0;
  int blen = 0;

  blen = snprintf(bf, MAX_BUFFER, "%i", gpio);
  if (blen < 0) {
    return -1;
  }

  fd = open("/sys/class/gpio/export", O_WRONLY);
  if (fd < 0) {
    return -2;
  }

  if (write(fd, bf, blen) < blen) {
    rv = -3;
  }

  do {
    rv = close(fd);
  } while ((rv < 0) && (errno == EINTR));

  return rv;
}

static int direction(int gpio, char output) {
  int fd;
  char fn[MAX_GPIO_FN];
  int rv = 0;

  if (snprintf(fn, MAX_GPIO_FN, "/sys/class/gpio/gpio%i/direction", gpio) < 0) {
    return -1;
  }

  fd = open(fn, O_WRONLY);
  if (fd < 0) {
    return -2;
  }

  if (output) {
    if (write(fd, "out\n", 4) < 4) {
      rv = -3;
    }
  } else {
    if (write(fd, "in\n", 3) < 3) {
      rv = -3;
    }
  }

  do {
    rv = close(fd);
  } while ((rv < 0) && (errno == EINTR));

  return rv;
}

static int setup(int gpio, char output) {
  int rv = 0;

  rv = export(gpio);
  if (rv < 0) {
    return rv;
  }

  rv = direction(gpio, output);
  if (rv < 0) {
    return rv;
  }

  return 0;
}

static int set(int gpio, char state) {
  char fn[MAX_GPIO_FN];
  int rv = 0;

  if (snprintf(fn, MAX_GPIO_FN, "/sys/class/gpio/gpio%i/value", gpio) < 0) {
    return -1;
  } else {
    int fd = open(fn, O_WRONLY);

    if (fd) {
      (void)write(fd, state ? "1\n" : "0\n", 2);
      /* we ignore the return value of that write because we use this in a loop
         anyway, and it's quite unlikely to fail. */

      do {
        rv = close(fd);
      } while ((rv < 0) && (errno == EINTR));
    }
  }

  return rv;
}

static int pulse(int gpio, int period, int duration) {
  if (set(gpio, 1) != 0) {
    return -1;
  }

  (void)usleep(duration);
  /* we ignore usleep()'s return value, because the only error would be to be
     interrupted by a signal, which is OK. */

  if (set(gpio, 0) != 0) {
    return -2;
  }

  (void)usleep(period - duration);

  return 0;
}

int main(int argc, char **argv) {
  if (setup(22, 1) != 0) {
    printf("Could not set up pin #22 as an output pin for the pulse train.\n");

    return -1;
  }

  /* create a pulse train with the same modulation as the PIco's FSSD. */
  while (1) {
    (void)pulse(22, 500000, 250000);
    /* note how we don't use the return value here, because we'd really just
     * send another pulse. */
  }

  return 0;
}