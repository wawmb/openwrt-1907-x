#ifdef __cplusplus
extern "C"
{
#endif

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdlib.h>

#include <pci/pci.h>

// for dir operations
#include <dirent.h>

#include "fuxipci_comm.h"
#include "fuxipci_device.h"
#include "fuxipci_lpbk.h"
#include "fuxipci_err.h"

#if 0
typedef struct _pci_dev_id {
    u16 ven_id;
    u16 dev_id;
} pci_dev_id;
#endif

	typedef struct yt_pci_dev_s
	{
		char df_path[50]; // debug fs path
		u16 domain;
		u16 ven_id;
		u16 dev_id;
		u16 sub_ven_id;
		u16 sub_dev_id;
		u32 mem;
		u8 bus, dev, func;
		struct pci_dev *pcidev;
	} yt_pci_dev_t;

#if 0
pci_dev_id dev_list[] = {
	{ 0x1f0a, 0x6801 },
};
#else
	extern pci_dev_id dev_list[];
#endif

/* PCI devide releated */
#define YT6801_MAX_NUM_SUPPORTED 10
	extern yt_pci_dev_t yt6801_devices[YT6801_MAX_NUM_SUPPORTED];
	extern int num_of_yt6801_dev;
	extern int cur_yt6801_dev;
	// extern pci_dev_id dev_list[]; //in file: fuxipci_lpbk.cpp

	int findout_all_yt6801_devices(int verbose)
	{
		int i = 0;
		int ret = 0;
		struct pci_access *tmp_access;
		struct pci_dev *tmp_dev = NULL;
		u32 val;

		tmp_access = pci_alloc();
		pci_init(tmp_access);

		if (verbose)
			tmp_access->debugging = 1;
		i = 0;
		pci_scan_bus(tmp_access);
		do
		{
			if (tmp_access->devices)
			{
				if (NULL == tmp_dev)
					tmp_dev = tmp_access->devices;
				if ((tmp_dev->vendor_id == dev_list[0].ven_id) && (tmp_dev->device_id == dev_list[0].dev_id))
				{
					if (NULL == yt6801_devices[num_of_yt6801_dev].pcidev)
					{
						// get the first device
						yt6801_devices[num_of_yt6801_dev].pcidev = tmp_dev;
					}
					yt6801_devices[num_of_yt6801_dev].domain = tmp_dev->domain_16;
					yt6801_devices[num_of_yt6801_dev].bus = tmp_dev->bus;
					yt6801_devices[num_of_yt6801_dev].dev = tmp_dev->dev;
					yt6801_devices[num_of_yt6801_dev].func = tmp_dev->func;
					yt6801_devices[num_of_yt6801_dev].mem = pci_read_long(tmp_dev, 0x10) & (~0xf);
					yt6801_devices[num_of_yt6801_dev].dev_id = tmp_dev->device_id;
					yt6801_devices[num_of_yt6801_dev].ven_id = tmp_dev->vendor_id;
					yt6801_devices[num_of_yt6801_dev].sub_dev_id = pci_read_word(tmp_dev, 0x2e);
					;
					yt6801_devices[num_of_yt6801_dev].sub_ven_id = pci_read_word(tmp_dev, 0x2c);

					if (verbose)
					{
						val = pci_read_long(tmp_dev, 0);
						printf("[%d] confirm\tid-%04x, bdf=%04x:%02x:%02x.%02x, mem=0x%08x,sub(v=%04x, d=%04x).\n", i, val, tmp_dev->domain_16, tmp_dev->bus, tmp_dev->dev, tmp_dev->func,
							   yt6801_devices[num_of_yt6801_dev].mem,
							   yt6801_devices[num_of_yt6801_dev].sub_ven_id,
							   yt6801_devices[num_of_yt6801_dev].sub_dev_id);
					}

					num_of_yt6801_dev++;
					if (num_of_yt6801_dev >= YT6801_MAX_NUM_SUPPORTED)
					{
						printf("Reached the max number of supported devices, %d of %d.\n", num_of_yt6801_dev, YT6801_MAX_NUM_SUPPORTED);
						break;
					}
				}
				else if (0 && verbose)
				{
					val = pci_read_long(tmp_dev, 0);
					printf("[%d] \t\tid-%04x, bdf=%04x:%02x:%02x.%02x\n", i, val, tmp_dev->domain_16, tmp_dev->bus, tmp_dev->dev, tmp_dev->func);
				}
				tmp_dev = tmp_dev->next;
				i++;
			}

		} while (tmp_access->devices && tmp_dev /*->next*/);

		pci_cleanup(tmp_access);

		for (i = 0; i < num_of_yt6801_dev; i++)
		{
			sprintf(yt6801_devices[i].df_path, "/sys/kernel/debug/fuxi_%d/fuxi-gmac/", num_of_yt6801_dev - 1 - i);
			if (verbose)
			{
				printf("[%d] debugfs = %s.\n", i, yt6801_devices[i].df_path);
			}
		}

		return num_of_yt6801_dev;
	}

	// block for PCI-net map info
	// output: name_net_dev
	// output: bdf
	char *get_netdevice_name_by_bdf(int domain, int bus, int dev, int func, char *bdf, char *name_net_dev)
	{
		char dname[128] = {0};
		DIR *dirnet;
		struct dirent *dirnetname;

		if (NULL == name_net_dev)
			return name_net_dev;
		if (NULL == bdf)
			return bdf;

		sprintf(bdf, "%04x:%02x:%02x.%01d", domain, bus, dev, func);
		sprintf(dname, "/sys/bus/pci/devices/%s/net/", bdf);
		dirnet = opendir(dname);
		if (dirnet)
		{
			while ((dirnetname = readdir(dirnet)) != 0)
			{
				if ('.' == dirnetname->d_name[0])
					continue;

				snprintf(name_net_dev, 50 - 1, "%s", dirnetname->d_name);
			}
			closedir(dirnet);
		}
		else
			return NULL;

		// printf("pci %s map to net device:%s\n", bdf, name_net_dev);
		return name_net_dev;
	}

#ifdef __cplusplus
}
#endif
