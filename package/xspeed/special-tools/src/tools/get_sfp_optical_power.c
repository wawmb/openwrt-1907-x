#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <linux/i2c-dev.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

float hex_to_float(unsigned int hex)
{
	union
	{
		unsigned int u32;
		float f;
	} converter;
	converter.u32 = hex;
	return converter.f;
}

int main(int argc, char *argv[])
{
	int file, i, ret;
	float reg, value;
	unsigned int rx_pwr_ad;
	unsigned int ad_reg = 0x68, base_reg = 0x38;
	unsigned char data[4];
	unsigned int merged[5];
	float merged_f[5];
	char cmd[32] = {0};

	if (argc < 2)
	{
		printf("eserial <port>\n");
		printf("For example: ./eserial 0\n");
		return -1;
	}
	i = atoi(argv[1]);
	printf("Port number: %d\n", i);

	sprintf(cmd, "i2cset -f -y 0 0x71 0 %d", 1 << i);
	ret = system(cmd);
	if (ret != 0)
		return -1;

	if ((file = open("/dev/i2c-0", O_RDWR)) < 0)
	{
		perror("Failed to open I2C bus");
		return -1;
	}
	if (ioctl(file, I2C_SLAVE, 0x51) < 0)
	{
		perror("Failed to set I2C slave address");
		close(file);
		return -1;
	}
	if (write(file, &ad_reg, 1) != 1)
	{
		perror("Failed to write register address");
		close(file);
		return -1;
	}
	if (read(file, data, 2) != 2)
	{
		perror("Failed to read data");
		close(file);
		return -1;
	}
	rx_pwr_ad = (data[0] << 8) | data[1];
	if (rx_pwr_ad <= 0x1)
	{
		printf("optical power: no detected\n");
		close(file);
		return -1;
	}
	for (int i = 0; i < 5; i++, base_reg += 4)
	{
		if (write(file, &base_reg, 1) != 1)
		{
			perror("Failed to write register address");
			close(file);
			return -1;
		}
		if (read(file, data, 4) != 4)
		{
			perror("Failed to read data");
			close(file);
			return -1;
		}
		merged[i] = (data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3];
		merged_f[i] = hex_to_float(merged[i]);
		// printf("Raw data: ");
		// for (int i = 0; i < 4; i++) {
		// 	printf("0x%02X ", data[i]);
		// }
		// printf("\n merged/merged_f: %08x/%.12f\n", merged[i], merged_f[i]);
	}
	/* Convert mw to dbm */
	reg = pow((float)rx_pwr_ad, 4) * merged_f[0] + pow((float)rx_pwr_ad, 3) * merged_f[1] +
		  pow((float)rx_pwr_ad, 2) * merged_f[2] + pow((float)rx_pwr_ad, 1) * merged_f[3] +
		  merged_f[4];

	value = log10(reg / 100000) * 10;
	printf("reg:%02f, optical power: %02f dbm\n", reg, value);
	close(file);

	return 0;
}