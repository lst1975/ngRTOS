/*************************************************************************************
 *                               ngRTOS Kernel V2.0.1
 * Copyright (C) 2022 Songtao Liu, 980680431@qq.com.  All Rights Reserved.
 **************************************************************************************
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * THE ABOVE COPYRIGHT NOTICE AND THIS PERMISSION NOTICE SHALL BE INCLUDED IN ALL
 * COPIES OR SUBSTANTIAL PORTIONS OF THE SOFTWARE. WHAT'S MORE, A DECLARATION OF 
 * NGRTOS MUST BE DISPLAYED IN THE FINAL SOFTWARE OR PRODUCT RELEASE. NGRTOS HAS 
 * NOT ANY LIMITATION OF CONTRIBUTIONS TO IT, WITHOUT ANY LIMITATION OF CODING STYLE, 
 * DRIVERS, CORE, APPLICATIONS, LIBRARIES, TOOLS, AND ETC. ANY LICENSE IS PERMITTED 
 * UNDER THE ABOVE LICENSE. THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF 
 * ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF 
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO 
 * EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES 
 * OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS 
 * IN THE SOFTWARE.
 *
 *************************************************************************************
 *                              https://www.ngRTOS.org
 *                              https://github.com/ngRTOS
 **************************************************************************************
 */

#ifndef __ngRTOS_IOCTL_H__
#define __ngRTOS_IOCTL_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Disk Status Bits (DSTATUS) */

#define NG_DEV_STA_NOINIT   0x01  /* Drive not initialized */
#define NG_DEV_STA_NODISK   0x02  /* No medium in the drive */
#define NG_DEV_STA_PROTECT  0x04  /* Write protected */

/* Generic command (Not used by FatFs) */

/* Generic command (Used by FatFs) */
/* Makes sure that the device has finished pending write process. If the disk 
   I/O layer or storage device has a write-back cache, the dirty cache data 
   must be committed to media immediately. Nothing to do for this command if 
   each write operation to the media is completed within the disk_write 
   function. Complete pending write process (needed at FF_FS_READONLY == 0) */
#define NG_DEV_CTRL_SYNC         0  
/* Get media size (needed at FF_USE_MKFS == 1) */
#define NG_DEV_GET_SECTOR_COUNT  1  
/* Get sector size (needed at FF_MAX_SS != FF_MIN_SS) */
#define NG_DEV_GET_SECTOR_SIZE   2  
/* Get erase block size (needed at FF_USE_MKFS == 1) */
#define NG_DEV_GET_BLOCK_SIZE    3  
/* Inform device that the data on the block of sectors is no longer used 
  (needed at FF_USE_TRIM == 1) */
#define NG_DEV_CTRL_TRIM         4  
/* Puts the device idle state. NG_DEV_STA_NOINIT in the current status flags may 
   not be set if the device goes active state by generic read/write function. */
#define NG_DEV_CTRL_POWER_IDLE   5  
/* Set power status. Puts the device off state. Shut-down the power to the device 
   and deinitialize the device interface if needed. NG_DEV_STA_NOINIT in the 
   current status flags must be set. The device goes active state by 
   disk_initialize function.*/
#define NG_DEV_CTRL_POWER_OFF    6  
/* Locks media eject mechanism. Lock media removal. */
#define NG_DEV_CTRL_LOCK         7  
/* Unlocks media eject mechanism. */
#define NG_DEV_CTRL_UNLOCK
/* Ejects media cartridge. NG_DEV_STA_NOINIT and NG_DEV_STA_NODISK in status 
   flag are set after the function succeeded.*/
#define NG_DEV_CTRL_EJECT        8 
/* Creates a physical format on the media. If buff is not null, it is pointer 
   to the call-back function for progress notification. */
#define NG_DEV_CTRL_FORMAT       9  /* Create physical format on the media */

/* Command code for disk_ioctrl fucntion */

#define NG_DEV_GET_DISK_STATUS   10  /* Get erase block size (needed at FF_USE_MKFS == 1) */
#define NG_DEV_GET_CARD_ISEXIST  11  /* To check whether the card is inserted or not */

#define NG_DEV_RTC_SET_TIME      0
#define NG_DEV_RTC_SET_DATE      1
#define NG_DEV_RTC_GET_DATE_TIME 2
#define NG_DEV_RTC_SET_DATE_TIME 3

struct sdio_ioctl_var {
  uint32_t val;
};
typedef struct sdio_ioctl_var sdio_ioctl_var_s;

#ifdef __cplusplus
}
#endif

#endif /* __ngRTOS_IOCTL_H__ */

