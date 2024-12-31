#ifndef __CRC_H__
#define __CRC_H__

#define CRC16_POLYNOM 0xA001     /* CRC16多项式, 默认:0x8005 */
#define CRC16_PRESET 0x0         /* CRC16校验预置值 */
#define CRC32_POLYNOM 0x04C11DB7 /* CRC32多项式, invert:0xEDB88320 */
#define CRC32_PRESET 0xFFFFFFFF  /* CRC32校验预置值 */

unsigned short calc_crc16(const void *src_buf, unsigned int bytelen,
                          unsigned short pre_value);

unsigned int Calc_crc32(const unsigned char *buf, unsigned int size);

int test_crc(void *p);

#endif /* __CRC_H__ */
