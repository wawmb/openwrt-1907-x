XSPEED_MENU:=X-Speed Kernel Modules

define KernelPackage/marvell_dsa
  CATEGORY:=$(XSPEED_MENU)
  TITLE:=Marvell NXP Support motorcomm dsa 617x mv88e6xxx
  KCONFIG:=CONFIG_NET_DSA CONFIG_NET_DSA_MV88E6XXX CONFIG_MARVELL_PHY CONFIG_MOTORCOMM_PHY CONFIG_MARVELL_10G_PHY \
	         CONFIG_NET_DSA_MV88E6XXX_GLOBAL2=y CONFIG_NET_DSA_TAG_DSA=y CONFIG_NET_DSA_TAG_EDSA=y
  FILES:=$(LINUX_DIR)/drivers/net/dsa/mv88e6xxx/mv88e6xxx.ko $(LINUX_DIR)/net/dsa/dsa_core.ko \
         $(LINUX_DIR)/drivers/net/phy/marvell.ko $(LINUX_DIR)/drivers/net/phy/motorcomm.ko \
         $(LINUX_DIR)/drivers/net/phy/marvell10g.ko
  AUTOLOAD:=$(call AutoLoad,60,marvell motorcomm dsa_core mv88e6xxx marvell10g)
endef
define KernelPackage/marvell_dsa/description
  Kernel modules for Marvell(NMXX) and NXP(NHXX/NZXX),
  Including motorcomm dsa_core 617x mv88e6xxx marvell10g Ethernet Support.
endef
$(eval $(call KernelPackage,marvell_dsa))

define KernelPackage/fuxi
  CATEGORY:=$(XSPEED_MENU)
  TITLE:=Phytium Rockchip Support fuxi for yt6801 Ethernet
  KCONFIG:=CONFIG_FUXI
  FILES:=$(wildcard $(LINUX_DIR)/drivers/net/ethernet/motorcomm/*/fuxi.ko)
  AUTOLOAD:=$(call AutoLoad,60,fuxi)
endef
define KernelPackage/fuxi/description
  Kernel modules for Phytium(NTXX) and Rockchip(RAXX),
  There are multiple yt6801 that need to burn LEDs fuxi Ethernet Support.
endef
$(eval $(call KernelPackage,fuxi))

define KernelPackage/r8111
  CATEGORY:=$(XSPEED_MENU)
  TITLE:=Rockchip Support r8111 Ethernet
  KCONFIG:=CONFIG_R8111H
  FILES:=$(LINUX_DIR)/drivers/net/ethernet/realtek/r8111/r8111.ko
  AUTOLOAD:=$(call AutoLoad,60,r8111)
endef
define KernelPackage/r8111/description
  Kernel modules for Rockchip(RAXX),
  Including the r8111 network Support.
endef
$(eval $(call KernelPackage,r8111))

define KernelPackage/pgdrv
  CATEGORY:=$(XSPEED_MENU)
  TITLE:=Rockchip Support pgdrv for r8111 wirte mac
  KCONFIG:=CONFIG_PGDRV
  FILES:=$(LINUX_DIR)/drivers/net/ethernet/realtek/pgdrv.ko
  AUTOLOAD:=
endef
define KernelPackage/pgdrv/description
  Kernel modules for Rockchip(RAXX),
  Including pgdrv for wirte r8111 mac Support.
endef
$(eval $(call KernelPackage,pgdrv))