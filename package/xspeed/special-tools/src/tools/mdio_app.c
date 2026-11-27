#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>


#define help() \
    printf("mdio:\n"); \
    printf("read operation: mdio_app <bus 0-3> <phyaddr> <regaddr>\n"); \
    printf("write operation: mdio_app <bus 0-3> <phyaddr> <regaddr> <valaddr>\n"); \
    printf("For example:\n"); \
    printf("mdio_app 0 0x2 0x2\n"); \
    printf("mdio_app 0 0x2 0x00 0xxx\n\n"); \
    exit(0);

typedef struct s_mii_data {
	unsigned short bus;
	unsigned short phyId;
	unsigned short reg;
	unsigned short data;
} mii_data;


#define DEVICE "/dev/mdio-uio"


int fd = -1;

int mdio_open(void)
{
	fd = open(DEVICE, O_RDWR);
	if (!fd) {
		printf("Error: Cannot open device: %s, fd:%x\n", DEVICE, fd);
		return -1;
	}

	return 0;
}

void mdio_close(void)
{
	if(fd)
		close(fd);
}

int mdio_read(unsigned short devbus,
		   unsigned short devaddr, unsigned short regaddr)
{
	int ret;
	mii_data mii;

	mii.bus   = devbus;
	mii.phyId = devaddr;
	mii.reg   = regaddr;

	ret = read(fd, (char*)&mii, sizeof(mii));
	if (ret < 0) {
		printf("Error read SMI reg bus %d devAddr: %x, regAddr: %x\n",
			   devbus, devaddr, regaddr);
		return -1;
	}

	return (int)mii.data;
}

int mdio_write(unsigned short devbus,
		   unsigned short devaddr, unsigned short regaddr,  unsigned short data)
{
	int ret;
	mii_data mii;

	mii.bus   = devbus;
	mii.phyId = devaddr;
	mii.reg   = regaddr;
	mii.data  = data;

	ret = write(fd, (char*)&mii, sizeof(mii));
	if (ret < 0) {
		printf("Error write SMI reg bus %d devAddr: %x, regAddr: %x  data: %x\n",
			   devbus, devaddr, regaddr, data);
		return -1;
	}

	return ret;
}


int main(int argc, char *argv[])
{
	ushort value;
	ushort cmd_bus;
	ushort cmd_phyaddr;
	ushort cmd_regaddr;
	ushort cmd_reg;

    if ((argc < 4) || (argc > 5) || (!strncmp(argv[1], "-h", 2))) {
        help();
    }

	mdio_open();

	cmd_bus = (ushort)strtoul(argv[1], NULL, 0);
	cmd_phyaddr = (ushort)strtoul(argv[2], NULL, 0);
	cmd_regaddr = (ushort)strtoul(argv[3], NULL, 0);

    if (argc == 4) {
		value = mdio_read(cmd_bus, cmd_phyaddr, cmd_regaddr);

		printf("read in bus %d phyaddr 0x%x regaddr 0x%x valaddr 0x%x\n",
				cmd_bus, cmd_phyaddr, cmd_regaddr, value);

    } else {
		cmd_reg = (ushort)strtoul(argv[4], NULL, 0);
		value = mdio_write(cmd_bus, cmd_phyaddr, cmd_regaddr, cmd_reg);

		printf("write in bus %d phyaddr 0x%x regaddr 0x%x valaddr 0x%x\n",
				cmd_bus, cmd_phyaddr, cmd_regaddr, cmd_reg);
    }

	mdio_close();

	return 0;
}
