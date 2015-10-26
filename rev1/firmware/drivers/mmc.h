#ifndef __MMC_H__
#define __MMC_H__

uint8_t detectCard(void);
DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void *buff);

#endif
