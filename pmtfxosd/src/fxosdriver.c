/*
 * DOC: -- fxosdriver.c -- fxos8700 accelerometer/magnetometer on i2c driver
 *
 * Reads the x,y,z coordinates for fxos8700
 * magnetometer and accelerometer off the i2c2 bus
 * at i2c address 0x1e.
 *
 * There are 2 parts to this driver.
 *  - The i2c-1 bus driver - copied from
 *       http://elinux.org/Interfacing_with_I2C_Devices
 *       kernel sources/Documentation/i2c/dev-interface
 *  - The fxos8700 device driver - copied from
 *       Freescale FXOS8700CQ datasheet
 */

#include "pmtfxos.h"       /* for SRAWDATA */
#include <errno.h>         /* for error messages via errno */
#include <fcntl.h>         /* for O_RDWR = file control definitions, open */
#include <i2c/smbus.h>     /* for i2c_smbus_read_data  */
#include <linux/i2c-dev.h> /* for i2c_SLAVE */
#include <math.h>          /* for atan2() */
#include <stdint.h>        /* for uint_8 */
#include <stdio.h>         /* for printf()  */
#include <stdlib.h>        /* for exit() */
#include <string.h>        /* for strerror() */
#include <sys/ioctl.h>     /* for ioctl() */
#include <syslog.h>        /* for syslog */
#include <unistd.h>        /* for close() */

/**
 * MAINFORTESTING tests the following 4 functions...
 *  fd = i2cOpen()
 *  initFXOS8700(fd)
 *  ReadAccelMagnData(fd, &pAccelData, &pMagnData)
 *  i2cClose(fd)
 */
/* #define MAINFORTESTING */

#define N 50 /* for calibration in main() */

#define FAILURE 0
#define SUCCESS 1

/* FXOS8700 I2C address */
#define FXOS8700CQ_SLAVE_ADDR 0x1E // with pins SA0=0, SA1=0

/* FXOS8700 internal register addresses */
#define FXOS8700CQ_STATUS 0x00
#define FXOS8700CQ_WHOAMI 0x0D
#define FXOS8700CQ_XYZ_DATA_CFG 0x0E
#define FXOS8700CQ_CTRL_REG1 0x2A
#define FXOS8700CQ_M_CTRL_REG1 0x5B
#define FXOS8700CQ_M_CTRL_REG2 0x5C
#define FXOS8700CQ_WHOAMI_VAL 0xC7
#define I2C_ERROR -1
#define I2C_OK 0

/* number of bytes to be read from the FXOS8700 */
#define FXOS8700CQ_READ_LEN 13 /* status plus 6 channels = 13 bytes */

static char err[100];

/**
 * i2cOpen() -- open i2c bus
 * Return: file descriptor or FAILURE
 */
int i2cOpen(void) {
  int file;
  char filename[40];

  /* open bus */
  sprintf(filename, "/dev/i2c-1");
  if ((file = open(filename, O_RDWR)) < 0) {
    sprintf(err, "failed to open i2c-1 bus =  %s\n", strerror(errno));
    syslog(LOG_NOTICE, "%s", err); /*to /var/log/syslog */
    return FAILURE;
  }
  /* open device on the bus */
  if (ioctl(file, I2C_SLAVE, FXOS8700CQ_SLAVE_ADDR) < 0) {
    sprintf(err, "FXOS8700 address 0x1e not responding = %s\n",
            strerror(errno));
    syslog(LOG_NOTICE, "%s", err); /*to /var/log/syslog */
    return FAILURE;
  }
  return (file);
}

/**
 * s_i2c_read_regs() -- read fxos8700 registers
 * @fd file descriptor of i2c-1 bus opened earlier
 * @slave address 0x1e
 * @registerid start reading at register 0x00 to 0x6f
 * @*databyte buffer to receive data into
 * @count count of bytes read
 * Return: count of bytes read
 */
int s_i2c_read_regs(int fd, int slave, int registerid, uint8_t *databyte,
                    uint8_t count) {
  int i;

  for (i = 0; i < count; i++) {
    databyte[i] = i2c_smbus_read_byte_data(fd, registerid);
    /*	printf("R%x = %x   ",registerid, databyte[i]); */
    registerid++;
  }
  return (count);
}

/**
 * s_i2c_write_regs() -- write 1 byte to fxos8700 register
 * @fd file descriptor of i2c-1 bus opened earlier
 * @slave address 0x1e
 * @registerid start reading at register 0x00 to 0x6f
 * @*databyte buffer to receive data into
 * @count count of bytes read
 * Return: count of bytes read
 */
int s_i2c_write_regs(int fd, int slave, int registerid, uint8_t *databyte,
                     uint8_t count) {
  if (count != 1)
    syslog(LOG_NOTICE, "max 1 byte can be written\n");

  i2c_smbus_write_byte_data(fd, registerid, *databyte);
  //    printf("wrote %x into register %x\n",*databyte, registerid);

  return (1);
}

/**
 * i2cClose() -- close i2c bus
 * Return: nothing
 */
void i2cClose(int fd) {
  close(fd);
  return;
}

/**
 * initFXOS8700() -- configure fxos8700 accelerometer, magnetometer
 * @aFP file descriptor of i2c-1 bus opened earlier
 *
 * This code is copied verbatum from
 * Freescale FXOS8700CQ datasheet
 *   - printf() replaced with syslog()
 *   - comments reformated to kernel-docs standard
 *
 * Return: I2C-OK on success, I2C-ERROR on failure
 */
int initFXOS8700(int aFP) {
  /* FXOS8700 data structure for WRITE */
  uint8_t databyte;

  /* read and check the FXOS8700 WHOAMI register */
  if (s_i2c_read_regs(aFP, FXOS8700CQ_SLAVE_ADDR, FXOS8700CQ_WHOAMI, &databyte,
                      (uint8_t)1) != 1) {
    syslog(LOG_NOTICE, "FXOS8700 WHOAMI register invalid\n");
    return (I2C_ERROR);
  }
  if (databyte != FXOS8700CQ_WHOAMI_VAL) {
    syslog(LOG_NOTICE, "FXOS8700 value register invalid\n");
    return (I2C_ERROR);
  }
  /*
   * write 0000 0000 = 0x00 to accelerometer control register 1 to place
   * FXOS8700 into standby [7-1] = 0000 000 [0]: active=0
   */
  databyte = 0x00;
  if (s_i2c_write_regs(aFP, FXOS8700CQ_SLAVE_ADDR, FXOS8700CQ_CTRL_REG1,
                       &databyte, (uint8_t)1) != 1) {
    syslog(LOG_NOTICE, "FXOS8700 accelerometer register error\n");
    return (I2C_ERROR);
  }
  /*
   * write 0001 1111 = 0x1F to magnetometer control register 1
   * [7]: m_acal=0: auto calibration disabled
   * [6]: m_rst=0: no one-shot magnetic reset
   * [5]: m_ost=0: no one-shot magnetic measurement
   * [4-2]: m_os=111=7: 8x oversampling (for 200Hz) to reduce magnetometer noise
   * [1-0]: m_hms=11=3: select hybrid mode with accel and magnetometer active
   */
  databyte = 0x1F;
  if (s_i2c_write_regs(aFP, FXOS8700CQ_SLAVE_ADDR, FXOS8700CQ_M_CTRL_REG1,
                       &databyte, (uint8_t)1) != 1) {
    syslog(LOG_NOTICE, "FXOS8700 magnetometer register1 invalid\n");
    return (I2C_ERROR);
  }
  /*
   * write 0010 0000 = 0x20 to magnetometer control register 2
   * [7]: reserved
   * [6]: reserved
   * [5]: hyb_autoinc_mode=1 to map the magnetometer registers to follow the
   * accelerometer registers
   * [4]: m_maxmin_dis=0 to retain default min/max latching even though not used
   * [3]: m_maxmin_dis_ths=0
   * [2]: m_maxmin_rst=0
   * [1-0]: m_rst_cnt=00 to enable magnetic reset each cycle
   */
  databyte = 0x20;

  if (s_i2c_write_regs(aFP, FXOS8700CQ_SLAVE_ADDR, FXOS8700CQ_M_CTRL_REG2,
                       &databyte, (uint8_t)1) != 1) {
    syslog(LOG_NOTICE, "FXOS8700 magnetometer register2 invalid\n");
    return (I2C_ERROR);
  }
  /*
   * write 0000 0001= 0x01 to XYZ_DATA_CFG register
   * [7]: reserved
   * [6]: reserved
   * [5]: reserved
   * [4]: hpf_out=0
   * [3]: reserved
   * [2]: reserved
   * [1-0]: fs=01 for accelerometer range of +/-4g range with 0.488mg/LSB
   */
  databyte = 0x01;
  if (s_i2c_write_regs(aFP, FXOS8700CQ_SLAVE_ADDR, FXOS8700CQ_XYZ_DATA_CFG,
                       &databyte, (uint8_t)1) != 1) {
    syslog(LOG_NOTICE, "FXOS8700 register3 invalid\n");
    return (I2C_ERROR);
  }
  /*
   * write 0000 1101b = 0x0D to accelerometer control register 1
   * [7-6]: aslp_rate=00
   * [5-3]: dr=001=1 for 200Hz data rate (when in hybrid mode)
   * [2]: lnoise=1 for low noise mode
   * [1]: f_read=0 for normal 16 bit reads
   * [0]: active=1 to take the part out of standby and enable sampling
   */
  databyte = 0x0D;
  if (s_i2c_write_regs(aFP, FXOS8700CQ_SLAVE_ADDR, FXOS8700CQ_CTRL_REG1,
                       &databyte, (uint8_t)1) != 1) {
    syslog(LOG_NOTICE, "FXOS8700  register? invalid\n");
    return (I2C_ERROR);
  }

  /* normal return */
  syslog(LOG_NOTICE, "FXOS8700 init completed normally\n");
  return (I2C_OK);
}

/**
 * ReadAccelMagnData() -- fxos8700 read
 * @fp file descriptor of i2c-1 bus opened earlier
 * @*pAccelData = x,y,z coordinates of accelerometer
 * @*pMagnData = x,y,z coordinates of magetometer
 *
 * code copied from Freescale FXOS8700CQ datasheet
 * 13 bytes read from fxos8700 (see below)
 * Status Register: 0x00
 * Accelerometer Registers:  x=0x01+0x02, y=0x03+0x04, z=0x05+0x06
 * Magnetometer Registers:   x=0x33+0x34, y=0x35+0x36, z=0x37+0x38
 * M_CTRL_REG2 = auto-increment enables 0x06 to 0x33 jump
 *
 * Return: I2C-OK on success, I2C-ERROR on failure
 */
int ReadAccelMagnData(int fp, SRAWDATA *pAccelData, SRAWDATA *pMagnData) {
  uint8_t Buffer[FXOS8700CQ_READ_LEN]; /* read buffer */

  /* read FXOS8700CQ Accelerometer + Magnetometer Registers */
  if (s_i2c_read_regs(fp, FXOS8700CQ_SLAVE_ADDR, FXOS8700CQ_STATUS, Buffer,
                      FXOS8700CQ_READ_LEN) == FXOS8700CQ_READ_LEN) {
    /* copy the accelerometer byte data into 16 bit words */
    pAccelData->x = (Buffer[1] << 8) | Buffer[2];
    pAccelData->y = (Buffer[3] << 8) | Buffer[4];
    pAccelData->z = (Buffer[5] << 8) | Buffer[6];
    /* copy the magnetometer byte data into 16 bit words */
    pMagnData->x = (Buffer[7] << 8) | Buffer[8];
    pMagnData->y = (Buffer[9] << 8) | Buffer[10];
    pMagnData->z = (Buffer[11] << 8) | Buffer[12];

    /**
     * PeterHACK bugfix: magetometer registers not read in above code
     *  which is verbatum copy from datasheet.
     * hyb_autoinc_mode bit not set in M_CTRL_REG2 above?
     * re-read the registers
     */
    s_i2c_read_regs(fp, FXOS8700CQ_SLAVE_ADDR, 0x33, Buffer, 6);
    pMagnData->x = (Buffer[0] << 8) | Buffer[1];
    pMagnData->y = (Buffer[2] << 8) | Buffer[3];
    pMagnData->z = (Buffer[4] << 8) | Buffer[5];

  } else {
    /* return with error */
    syslog(LOG_NOTICE, "FXOS8700 read failed\n");
    return (I2C_ERROR);
  }

  /* normal return */
  return (I2C_OK);
}

#ifdef MAINFORTESTING
int main() {
  int i;
  int fd;
  int axmean, aymean, azmean, mxmean, mymean, mzmean;
  int axsd, aysd, azsd, mxsd, mysd, mzsd, dsd;
  float degrees, sumdegrees;
  int sdev[N][7]; /* standard deviation */

  SRAWDATA pAccelData;
  SRAWDATA pMagnData;

  axmean = aymean = azmean = mxmean = mymean = mzmean = 0;
  axsd = aysd = azsd = mxsd = mysd = mzsd = dsd = 0;
  degrees = sumdegrees = 0.0;

  printf(" initiate i2c-1 bus and device=0x1e   \n");
  if ((fd = i2cOpen()) == FAILURE) {
    printf("i2c failed to open - check syslog \n");
    exit(0);
  }

  if ((initFXOS8700(fd)) == I2C_ERROR) {
    printf("FXOS8700 not found - check syslog \n");
    exit(0);
  }

  /* ignore 1st 10 reads at beginning */
  for (i = 0; i < 10; i++) {

    ReadAccelMagnData(fd, &pAccelData, &pMagnData);
  }
  /* stats for calibration */
  for (i = 0; i < N; i++) {

    ReadAccelMagnData(fd, &pAccelData, &pMagnData);
    degrees = atan2(pMagnData.x, pMagnData.y) * 180 / M_PI;
    /* keep data in array for std dev calc at end */
    sdev[i][0] = pAccelData.x;
    sdev[i][1] = pAccelData.y;
    sdev[i][2] = pAccelData.z;
    sdev[i][3] = pMagnData.x;
    sdev[i][4] = pMagnData.y;
    sdev[i][5] = pMagnData.z;
    sdev[i][6] = degrees;
    /* compute sum for mean calc at end  */
    axmean += pAccelData.x;
    aymean += pAccelData.y;
    azmean += pAccelData.z;
    mxmean += pMagnData.x;
    mymean += pMagnData.y;
    mzmean += pMagnData.z;
    sumdegrees += degrees;
    /* print the data */
    printf("accel(m/s^2) = %d %d %d mag(uTesla) = %d %d %d degrees = %f  \n",
           pAccelData.x, pAccelData.y, pAccelData.z, pMagnData.x, pMagnData.y,
           pMagnData.z, degrees);
  }
  /* compute mean, std deviation */
  axmean = axmean / N;
  aymean = aymean / N;
  azmean = azmean / N;
  mxmean = mxmean / N;
  mymean = mymean / N;
  mzmean = mzmean / N;
  degrees = sumdegrees / N;
  for (i = 0; i < N; i++) {
    axsd += pow((sdev[i][0] - axmean), 2);
    aysd += pow((sdev[i][1] - aymean), 2);
    azsd += pow((sdev[i][2] - azmean), 2);
    mxsd += pow((sdev[i][3] - mxmean), 2);
    mysd += pow((sdev[i][4] - mymean), 2);
    mzsd += pow((sdev[i][5] - mymean), 2);
    dsd += pow((sdev[i][6] - mzmean), 2);
  }
  /* print mean, std deviation */
  printf("accel(mean ) = %d %d %d mag(mean  ) = %d %d %d deg-avg = %d  \n",
         axmean, aymean, azmean, mxmean, mymean, mzmean, (int)degrees);
  printf("accel(sdev) = %d %d %d mag(sdev) = %d %d %d deg-sdev = %d  \n",
         (int)sqrt(axsd / N), (int)sqrt(aysd / N), (int)sqrt(azsd / N),
         (int)sqrt(mxsd / N), (int)sqrt(mysd / N), (int)sqrt(mzsd / N),
         (int)sqrt(dsd / N));
  i2cClose(fd);
  return 0;
}

/* see ../../Cross Compile instructions  for compile */
#endif
