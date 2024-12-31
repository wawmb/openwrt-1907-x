// SPDX-License-Identifier: GPL-3.0+
/* Copyright (c) 2021 Motor-comm Corporation.
 * Confidential and Proprietary. All rights reserved.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include "io_helper.h"

#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>

#ifdef __x86_64__
#include <sys/io.h>
#else
#include <sys/uio.h>
#endif


int MemRead(uint64_t uPhysAddr, void *pBuf, size_t nSize)
{
	int ret = 0;

	if (nSize == 0)
		return ret;

	int fd = open("/dev/mem", O_RDONLY | O_SYNC);
	if (fd == -1)
	{
		perror("open memory device failed:");
		return errno;
	}

	const long page_size = sysconf(_SC_PAGE_SIZE);

	uint32_t uPhysAddrOffset = uPhysAddr % page_size;
	uint64_t uBasePhysAddr = uPhysAddr - uPhysAddrOffset;

	size_t read_size = 0;

	do{
		size_t read_max = page_size - uPhysAddrOffset;
		size_t rest_size = nSize - read_size;
		size_t copy_size = (rest_size > read_max) ? read_max : rest_size;

		unsigned char* map_addr = (char*)mmap(NULL, page_size, PROT_READ, MAP_SHARED, fd, uBasePhysAddr);

		if (map_addr == MAP_FAILED) {
			perror("mmap failed:");
			close(fd);
			return -1;
		}

		memcpy(pBuf + read_size, map_addr + uPhysAddrOffset, copy_size);

		if ((ret = munmap(map_addr, page_size)) != 0) {
			perror("munmap failed:");
			close(fd);
			return ret;
		}

		read_size += copy_size;
		uPhysAddrOffset = (uPhysAddrOffset + copy_size) % page_size;	// if cycle continue, offset always 0
		uBasePhysAddr += page_size;	// every cycle read a memory page

	} while (read_size < nSize);

	close(fd);
	
	return ret;
}

int MemRead8(uint64_t uPhysAddr, uint8_t *pValue)
{
	return MemRead(uPhysAddr, (void *)pValue, 1);
}

int MemRead16(uint64_t uPhysAddr, uint16_t *pValue)
{
	return MemRead(uPhysAddr, (void *)pValue, 2);
}

int MemRead32(uint64_t uPhysAddr, uint32_t *pValue)
{
	return MemRead(uPhysAddr, (void *)pValue, 4);
}

int MemWrite(uint64_t uPhysAddr, void *pBuf, size_t nSize)
{
	int ret = 0;

	if (nSize == 0)
		return ret;

	int fd = open("/dev/mem", O_RDWR | O_SYNC);
	if (fd == -1)
	{
		perror("open memory device failed:");
		return errno;
	}
	
	const long page_size = sysconf(_SC_PAGE_SIZE);

	uint32_t uPhysAddrOffset = uPhysAddr % page_size;
	uint64_t uBasePhysAddr = uPhysAddr - uPhysAddrOffset;

	size_t write_size = 0;

	do {
		size_t write_max = page_size - uPhysAddrOffset;
		size_t rest_size = nSize - write_size;
		size_t copy_size = (rest_size > write_max) ? write_max : rest_size;

		unsigned char* map_addr = (char*)mmap(NULL, page_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, uBasePhysAddr);

		if (map_addr == MAP_FAILED) {
			perror("mmap failed:");
			close(fd);
			return -1;
		}

		memcpy(map_addr + uPhysAddrOffset, pBuf + write_size, copy_size);

		if ((ret = munmap(map_addr, page_size)) != 0) {
			perror("munmap failed:");
			close(fd);
			return ret;
		}

		write_size += copy_size;
		uPhysAddrOffset = (uPhysAddrOffset + copy_size) % page_size;	// if cycle continue, offset always 0
		uBasePhysAddr += page_size;	// every cycle read a memory page

	} while (write_size < nSize);

	close(fd);

	return ret;
}

int MemWrite8(uint64_t uPhysAddr, uint8_t uValue)
{
	return MemWrite(uPhysAddr, (void *)&uValue, 1);
}

int MemWrite16(uint64_t uPhysAddr, uint16_t uValue)
{
	return MemWrite(uPhysAddr, (void *)&uValue, 2);
}

int MemWrite32(uint64_t uPhysAddr, uint32_t uValue)
{
	return MemWrite(uPhysAddr, (void *)&uValue, 4);
}


