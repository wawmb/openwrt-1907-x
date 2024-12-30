/*++

Copyright (c) 2022  Motorcomm Corporation

Module Name:

    fuxipci_linux_driver.cpp

Abstract:

    FUXI Linux driver function

Environment:

    User mode.

Revision History:
    2022-03-15    xiaogang.fan@motor-comm.com    created

--*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <net/if.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "fuxipci_comm.h"
#include "fuxipci_device.h"
#include "fuxipci_net.h"
#include "fuxipci_lpbk.h" 
#include "fuxipci_linux_driver.h"
#include "fuxipci_err.h"
#include "fuxipci_linux_msc.h"
#include "fuxipci_linux_hwcom.h"
#include "fuxipci_l1f_hw.h"

ADAPTER ADPT;
char gFuxiDfPath[50];

int test_bist(PADAPTER pAdapter)
{
    int ret, i;
    u32 ret0, ret1;
    
    printf("SRAM Bist Test ....................................\n");
    
    /* reset MAC */
    if (!ioctl_diag_device_reset(pAdapter))
    {
        printf("FATAL ERROR: Reset MAC failed during bist test!\n");
        return ERR_RST_MAC;
    }
    
    /* issue bist command */
    MEM_W32(pAdapter, L1F_BIST0, L1F_BIST0_START);
    MEM_W32(pAdapter, L1F_BIST1, L1F_BIST1_START);

    
    // delay max for 20ms
    ret0 = ret1 = 1;
    for (i=0; i<5; i++)
    {
        MS_DELAY(pAdapter, 4);   //MS_DELAY(pAdapter, 4);
        MEM_R32(pAdapter, L1F_BIST0, &ret0);
        MEM_R32(pAdapter, L1F_BIST1, &ret1);
        
        if ((ret0 & L1F_BIST0_START)  || (ret1 & L1F_BIST1_START))
            continue;
        else
            break;
    }
    
    if ((ret0 & L1F_BIST0_START) == 0 && (ret1 & L1F_BIST1_START) == 0 && 
        (ret0 & L1F_BIST0_FAIL) == 0 && (ret1 & L1F_BIST1_FAIL) == 0)
    {
        printf("\tAll.O.K.\n");
        return 0;
    }
    
    
    if (ret0 & L1F_BIST0_START)
    {
        printf("\t0.Not complete.");
        ret = ERR_BIST_NOT_COMPLETE;
    }
    else 
    {
        if (ret0 & L1F_BIST0_FAIL)
        {
            ret = ERR_BIST_FAILED;
            printf("\t0.Failed, test pat=%d step=%d Row=%d Col=%d",
                    (ret0>>4)&0x7,
                    (ret0>>8)&0xF,
                    (ret0>>12)&0xFFF,
                    (ret0>>24)&0x3F);
        }
        else
        {
            ret = 0;
            printf("\t0.Pass.");
        }
    }
    
    printf("\n");
    
    if (ret1 & L1F_BIST1_START)
    {
        printf("\t1.Not complete.");
        ret = ERR_BIST_NOT_COMPLETE;
    }
    else 
    {
        if (ret1 & L1F_BIST1_FAIL)
        {
            ret = ERR_BIST_FAILED;
            printf("\t1.Fail, test pat=%d step=%d Row=%d Col=%d",
                        (ret1>>4)&0x7,
                        (ret1>>8)&0xF,
                        (ret1>>12)&0x3F,
                        (ret1>>24)&0x1F);
        }
        else
        {
            printf("\t1.Pass.");
        }
    }
    
    printf("\n");
        
    return ret; 
}

int RegViaMem(PADAPTER pAdapter, PRW_REG pRwReg, u32 len)
{
    if (len != sizeof(*pRwReg))
    {
        printf("Input parameter error!\n");
        return STATUS_FAILED;
    }

    if(pRwReg->bRead)
    {
        MEM_R32(pAdapter, pRwReg->reg, &pRwReg->val);
    }
    else
    {
        MEM_W32(pAdapter, pRwReg->reg, pRwReg->val);
    }
    return STATUS_SUCCESSFUL;
}

int RegViaIo(PADAPTER pAdapter, PRW_REG pRwReg, u32 len)
{
    if (len != sizeof(*pRwReg))
    {
        printf("Input parameter error!\n");
        return STATUS_FAILED;
    }

    if (pRwReg->bDirect)
    {
        if (pRwReg->bRead)
        {
            IO_R32(pAdapter, pRwReg->reg, &pRwReg->val);
        }
        else
        {
            IO_W32(pAdapter, pRwReg->reg, pRwReg->val);
        }
    }
    else
    {
        if(pRwReg->bRead)
        {
            IO_W32(pAdapter, L1F_IO_ADDR, pRwReg->reg);
            IO_R32(pAdapter, L1F_IO_DATA, &pRwReg->val);
        }
        else
        {
            IO_W32(pAdapter, L1F_IO_ADDR, pRwReg->reg);
            IO_W32(pAdapter, L1F_IO_DATA, pRwReg->val);
        }
    }
    
    return STATUS_SUCCESSFUL;
}

void ShowStatistics(PADAPTER pAdapter)
{
    u32 val;
    u16 reg;

    MEM_R32(pAdapter, 0x1480, &val);
    //DBG_PRINT(("reg1480=0x%08x, enCutThru=%d\n", val, pAdapter->enCutThru));
    DBG_PRINT(("reg1480=0x%08x\n", val));

    for (reg=0x1700; reg<=0x17C0; reg+=4)
    {
        MEM_R32(pAdapter, reg, &val);
        DBG_PRINT(("Reg%X=%-8x    ", reg, val));
    }

    //printf("SW TxTotalPacket=%d, RxTotalPacket=0x%08lX, ",
    //      pAdapter->TxTotalPacket, pAdapter->RxTotalPacket);

}

void ClearStatistics(PADAPTER pAdapter)
{
    u32 val;
    u16 reg;

    for (reg=0x1700; reg<=0x17C0; reg+=4)
    {
        MEM_R32(pAdapter, reg, &val);
    }
}

//******************************************************
bool HwInitPHY2PhyLoopback(PADAPTER pAdapter, u8 LinkMode)
{
    int index;
    u16 phydata = 0;
#if 0
    // fix channel
    if (PHY_W(pAdapter, 16, 0x800))
        return false;
    if (LinkMode == LINKMODE_PHYLP_100F ||
        LinkMode == LINKMODE_PHYLP_100H ||
        LinkMode == LINKMODE_PHYLP_10F ||
        LinkMode == LINKMODE_PHYLP_10H)
    {
        // auto-negotiation first
        PHY_W(pAdapter, L1F_MII_BMCR, 0x9200);

        /* delay 1.5 seconds */
        for (index=0; index<15; index++)
            MS_DELAY(pAdapter, 100); // 100ms
    }
#endif
    // set 10BT/100BT/1000BT external loopback
    switch (LinkMode)
    {
    case LINKMODE_PHYLP_1000F:
        //if (PHY_W(pAdapter, 29, 0x0011) ||
        //    PHY_W(pAdapter, 30, 0x5553) ||
        if(PHY_W(pAdapter, L1F_MII_BMCR,  0x4140))
        {
            printf("Config 1000MF(mii) failed !\n");
        }
        break;

    case LINKMODE_PHYLP_100F:
        PHY_W(pAdapter, L1F_MII_BMCR, 0xA100);
        break;
    
    case LINKMODE_PHYLP_100H:
        PHY_W(pAdapter, L1F_MII_BMCR, 0xA000);
        break;
    
    case LINKMODE_PHYLP_10F:
        PHY_W(pAdapter, L1F_MII_BMCR, 0x8100);
        break;
    
    case LINKMODE_PHYLP_10H:
        PHY_W(pAdapter, L1F_MII_BMCR, 0x8000);
        break;

    case LINKMODE_MACLP_100F:
        PHY_W(pAdapter, L1F_MII_BMCR, 0xA100);
        MS_DELAY(pAdapter, 50);
        return true;

    case LINKMODE_MACLP_1000F:
    case LINKMODE_MACLP_100H:
    case LINKMODE_MACLP_10F:
    case LINKMODE_MACLP_10H:
    default:
        MS_DELAY(pAdapter, 10);
        return true;

    }


    /* delay 1.5 seconds first */
    for (index=0; index<15; index++)
        MS_DELAY(pAdapter, 100); // 100ms

    
    for (index=0; index<100; index++) 
    {
        MS_DELAY(pAdapter, 50); // delay 50ms

        if (!PHY_R(pAdapter, L1F_MII_BMSR, &phydata))
            continue;
    
        if (0 != (phydata & L1F_BMSR_LINK_STATUS))
        {
            //printf("PHY: link is up\n");
            MS_DELAY(pAdapter, 50);
            return true;
        }
    }

    printf("PHY-Status=%X\n", phydata);
    printf("Warning: check your External-Loopback-Connector !!\n");
    
    return false;
}

//******************************************************
bool HwInitPHY2CableLoopback(PADAPTER pAdapter, u8 LinkMode)
{
    int index;
    u16 phydata = 0;
    u32 regVal = 0;

    // set 10BT/100BT/1000BT external loopback
    switch (LinkMode)
    {
    case LINKMODE_CBLLP_1000F:
        if(PHY_W(pAdapter, L1F_MII_BMCR,  0x8140))
        {
            printf("Config 1000MF(mii) failed !\n");
        }
        break;

    case LINKMODE_PHYLP_100F:
        PHY_W(pAdapter, L1F_MII_BMCR, 0xA100);
        break;
    
    case LINKMODE_PHYLP_10F:
        PHY_W(pAdapter, L1F_MII_BMCR, 0x8100);
        break;
    
    default:
        MS_DELAY(pAdapter, 10);
        return true;

    }

    /* delay 1.5 seconds first */
    for (index=0; index<15; index++)
        MS_DELAY(pAdapter, 100); // 100ms

    
    for (index=0; index<100; index++) 
    {
        MS_DELAY(pAdapter, 50); // delay 50ms

        if (!PHY_R(pAdapter, L1F_MII_BMSR, &phydata))
            continue;
    
        if (0 != (phydata & L1F_BMSR_LINK_STATUS))
        {
            //printf("PHY: link is up\n");
            MS_DELAY(pAdapter, 50);
            return true;
        }
    }

    printf("PHY-Status=%X\n", phydata);
    printf("Warning: check your External-Loopback-Connector !!\n");
    
    return false;
}

//******************************************************

bool HwSetPhyReg2CableLoopbackMode(PADAPTER pAdapter){
    /* config cable loopback */
    if(PHY_W(pAdapter, 0x1e,  0x27))
    {
        printf("%s Config phy reg failed !\n", __func__);
    }
    if(PHY_W(pAdapter, 0x1f,  0x6812))
    {
        printf("%s Config phy reg failed !\n", __func__);
    }
    MS_DELAY(pAdapter, 5);

    if(PHY_W(pAdapter, 0x1e,  0xa))
    {
        printf("%s Config phy reg failed !\n", __func__);
    }
    if(PHY_W(pAdapter, 0x1f,  0x3a18))
    {
        printf("%s Config phy reg failed !\n", __func__);
    }
    MS_DELAY(pAdapter, 5);
}

bool HwResetPHY2NormalMode(PADAPTER pAdapter)
{
    int index;
    u16 phydata = 0;

    //if(pAdapter->LinkMode == LINKMODE_CBLLP_1000F||
    //        pAdapter->LinkMode == LINKMODE_CBLLP_100F||
    //        pAdapter->LinkMode == LINKMODE_CBLLP_10F)
    {
        if(PHY_W(pAdapter, 0x1e,  0x27))
        {
            printf("%s Config phy reg failed !\n", __func__);
        }
        if(PHY_W(pAdapter, 0x1f,  0xe812))
        {
            printf("%s Config phy reg failed !\n", __func__);
        }
        MS_DELAY(pAdapter, 5);

        if(PHY_W(pAdapter, 0x1e,  0xa))
        {
            printf("%s Config phy reg failed !\n", __func__);
        }
        if(PHY_W(pAdapter, 0x1f,  0x3a08))
        {
            printf("%s Config phy reg failed !\n", __func__);
        }
        MS_DELAY(pAdapter, 5);
    }
    
    if (PHY_W(pAdapter, L1F_MII_BMCR,  0x9140))
    {
        printf("Config 1000MF(mii) failed !\n");
    }

    /* delay 1.5 seconds first */
    for (index=0; index<15; index++)
        MS_DELAY(pAdapter, 100); // 100ms

    for (index=0; index<100; index++) 
    {
        MS_DELAY(pAdapter, 50); // delay 50ms

        if (!PHY_R(pAdapter, L1F_MII_BMSR, &phydata))
            continue;
    
        if (0 != (phydata & L1F_BMSR_LINK_STATUS))
        {
            //printf("PHY: link is up\n");
            MS_DELAY(pAdapter, 50);
            return true;
        }
    }
    
    return false;
}
   
// *************************************
u32 Open_Linux(
        char  *DevName
        )
{
    PADAPTER pAdapter = (PADAPTER)&ADPT;
    char file_name[256];

    memset(file_name, '\0', sizeof(file_name));
    snprintf(file_name, sizeof(file_name) - 1, "%s%s", gFuxiDfPath, DevName);
    pAdapter->debugfs_fd = open(file_name, O_RDWR);
    if (pAdapter->debugfs_fd < 0) {
        printf("Est debugfs ioctl fail. file_name is %s\n", file_name);
        return STATUS_FAILED;
    }
    return 0;
}

u32 Close_Linux()
{
    PADAPTER pAdapter = (PADAPTER)&ADPT;
    close(pAdapter->debugfs_fd);
    memset(pAdapter->if_name, 0, IFNAMSIZ);
    
    return 0;
}

PVOID Load_Linux(
        u16              NICPos,
        u16              Mode
        )
{
    PADAPTER pAdapter = (PADAPTER)&ADPT;
    u32 val;

    CFG_R32(pAdapter, EFUSE_REVID_REGISTER, &val);
    pAdapter->Mode = Mode;
    pAdapter->DevID = NICPos;
    pAdapter->RevID = (u8)(val & 0xFF);
    
    if (Mode == LOAD_MODE_NORMAL)
    {
        return pAdapter;
    }
    else if (Mode == LOAD_MODE_MAC_LOOPBACK)
    {
        pAdapter->LinkMode = LINKMODE_MACLP_1000F;
    }
    else if (Mode == LOAD_MODE_PHY_LOOPBACK)
    {
        pAdapter->LinkMode = LINKMODE_PHYLP_1000F;
    }
    else if (Mode == LOAD_MODE_CABLE_LOOPBACK)
    {
        pAdapter->LinkMode = LINKMODE_CBLLP_1000F;
    }
    
    if (ioctl_diag_start(pAdapter))
    {
        return pAdapter;
    }

    printf("Diag Start Failed.\n");
    return 0;
}

u32 Unload_Linux(
        PVOID               Driver
        )
{
    PADAPTER pAdapter = (PADAPTER)Driver;

    if (pAdapter->Mode == LOAD_MODE_NORMAL)
    {
        return 0;
    }

    if (ioctl_diag_end(pAdapter))
    {
        return 0;
    }

    return STATUS_FAILED;
}

u32 Reset_Linux(
        PVOID               Driver,
        bool                bActive
        )
{
    PADAPTER pAdapter = (PADAPTER)Driver;
    u32 rxqCtrl, txqCtrl, macCtrl, val;
    u16 phyVal;

    bActive = bActive;

    // Config PHY
    if(pAdapter->LinkMode == LINKMODE_PHYLP_1000F||
	    pAdapter->LinkMode == LINKMODE_PHYLP_100F||
	    pAdapter->LinkMode == LINKMODE_PHYLP_10F)
    {
    	if (!HwInitPHY2PhyLoopback(pAdapter, pAdapter->LinkMode)){
            printf("HwInitPhy2PhyLoopback fail \n");
            return ERR_INIT_PHY;
    	}
    }
    else if(pAdapter->LinkMode == LINKMODE_CBLLP_1000F||
	    pAdapter->LinkMode == LINKMODE_CBLLP_100F||
	    pAdapter->LinkMode == LINKMODE_CBLLP_10F)
    {
    	if (!HwInitPHY2CableLoopback(pAdapter, pAdapter->LinkMode)){
            printf("HwInitPhy2CableLoopback fail \n");
            return ERR_INIT_PHY;
    	}
    }
    else
    {
        printf("Error LinkMode paramters: %x, HwInitPhy fail.\n", pAdapter->LinkMode);
    }

    //enable promiscuous
    MEM_R32(pAdapter, 0x2008, &val);
    val |= 0x1;
    MEM_W32(pAdapter, 0x2008, val);

    //enable checksum
    //MEM_R32(pAdapter, 0x2000, &val);
    //val |= 0x8000000;
    //MEM_W32(pAdapter, 0x2000, val);

#if 0
    //disable PHY hibernation
    PHY_W(pAdapter, L1F_MII_DBG_ADDR, L1F_MIIDBG_HIBNEG);
    PHY_R(pAdapter, L1F_MII_DBG_DATA, &phyVal);
    phyVal &= ~(L1F_HIBNEG_HIB_PULSE | L1F_HIBNEG_PSHIB_EN);
    PHY_W(pAdapter, L1F_MII_DBG_DATA, phyVal);
#endif

    /* if mac loopback, set mac lm(loopback mode) and ipc(checksum offload) */
    if (pAdapter->Mode == LOAD_MODE_MAC_LOOPBACK) {
        /* read mac ctrl reg */
        MEM_R32(pAdapter, L1F_MAC_CTRL, &macCtrl);
        /* set lm */
        macCtrl |= L1F_MAC_CTRL_LPBACK_EN;
        /* set ipc */
        macCtrl |= L1F_MAC_CTRL_IPC_EN;
        /* write mac ctrl reg */
        MEM_W32(pAdapter, L1F_MAC_CTRL, macCtrl);
    } 

    pAdapter->bRegCfged = true;
    MS_DELAY(pAdapter, 100);
    return 0;
}

u32 Send_Linux(
        PVOID               Driver,
        PATHR_DOS_PACKET    PacketArry,
        u16                 NumPacket
        )
{
    PADAPTER pAdapter = (PADAPTER)Driver;
    PATHR_DOS_PACKET pSrcPkt, pDstPkt;
    int totalLen = 0;
    u8 *buf = NULL;
    u8 *pData;
    u32 ret = 0;

    if (NumPacket <= 0)
    {
        return STATUS_FAILED;
    }

    pSrcPkt = PacketArry;
    while (pSrcPkt)
    {
        if (pSrcPkt->Length != pSrcPkt->Buf[0].Length)
        {
            printf("Only support one packet one buffer\n");
            return STATUS_FAILED;
        }
        totalLen += pSrcPkt->Length;
        pSrcPkt = pSrcPkt->Next;
    }
    totalLen += sizeof(ATHR_DOS_PACKET) * NumPacket + sizeof(ext_ioctl_data);
    buf = (u8*)malloc(totalLen);
    pSrcPkt = PacketArry;
    pDstPkt = (PATHR_DOS_PACKET)(buf + sizeof(ext_ioctl_data));
    pData = (u8 *)pDstPkt + sizeof(ATHR_DOS_PACKET) * NumPacket;
    while (pSrcPkt)
    {
        memcpy(pDstPkt, pSrcPkt, sizeof(ATHR_DOS_PACKET));
        memcpy(pData, pSrcPkt->Buf[0].Addr, pSrcPkt->Length);

        pDstPkt->Buf[0].Addr = pData;
        pDstPkt->Buf[0].Offset = pData - buf;
        pDstPkt->Next = pDstPkt+1;

        pData += pSrcPkt->Length;
        pSrcPkt = pSrcPkt->Next;
        pDstPkt++;
    }
    // last packet
    pDstPkt--;
    pDstPkt->Next = NULL;
#if 0
    pData =  buf + sizeof(ATHR_DOS_PACKET) + sizeof(ext_ioctl_data);
    int tmpLen = ((ATHR_DOS_PACKET*)(buf + sizeof(ext_ioctl_data)))->Buf[0].Length;
    printf("Send packet len is %d, data:\n", tmpLen);
#if 1
    for(int i = 0; i < 70 ; i++)
    //for(int i = 0; i < tmpLen ; i++)
    {
        printf("%x ", pData[i]);
        if(i != 0 && i %8 == 0)
            printf("\n");
    }
    printf("\n");
#endif
#endif
    
    if (!ioctl_send_packets(pAdapter, buf, totalLen))
    {
        printf("Send packets fail.\n");
        ret = STATUS_FAILED;
    }
    free(buf);
    return ret;
}

u16 Receive_Linux(
        PVOID               Driver,
        PATHR_DOS_PACKET    pPacketArray,
        u32                 size
        )
{
    PADAPTER pAdapter = (PADAPTER)Driver;
    u8 *buf = (u8*)pPacketArray;
    PATHR_DOS_PACKET pPacket;
    u16 count = 0;
    u32 totalLen = 0;
    
    memset(buf, 0, size);
    if (!ioctl_receive_packets(pAdapter, buf, size))
    {
        return 0;
    }
    
    pPacket = (PATHR_DOS_PACKET)buf;
    while(pPacket->Length != 0)
    {
        count ++;
        pPacket->Buf[0].Addr = buf + pPacket->Buf[0].Offset;
        totalLen += pPacket->Length;
        if (pPacket->Next == NULL)
            break;
        pPacket = pPacket->Next;
        //pPacket->Next = pPacket + 1;
        //pPacket ++;
    }

 #if 0
    pPacket = (PATHR_DOS_PACKET)buf;
    printf("receive() packets num=%d, len=%d, next=%p \n", count, pPacket->Length, pPacket->Next);
    printf("data:\n");
    u8* pData = buf + sizeof(ATHR_DOS_PACKET);
    for(int i = 0; i < 50 ; i++)
    {
        printf("%x ", pData[i]);
        if(i != 0 && i %8 == 0)
            printf("\n");
    }
    printf("\n");
#endif
    return count;
}

u32 ReturnPackets_Linux(
        PVOID               Driver,
        PATHR_DOS_PACKET    PacketArry,
        u16                 NumPacket
        )
{
    Driver = Driver;
    PacketArry = PacketArry;
    NumPacket = NumPacket;
    return 0;
}

u32 GetStats_Linux(
        PVOID               Driver,
        PATHR_DOS_STATS     Stats
        )
{
    //PADAPTER pAdapter = (PADAPTER)Driver;
    Driver = Driver;
    Stats = Stats;
    //Stats->TxOK = pAdapter->TxTotalPacket;
    //Stats->RxOK = pAdapter->RxTotalPacket;
    return 0;
}

void NicSetParam(PADAPTER pAdapter, PSET_DEV_PARAM pParam)
{
    if(strcmp((char*)pParam->name, "RssEnable") == 0)
    {
        pAdapter->rxchkRSS = (bool)(pParam->val);
    }
    else if(strcmp((char*)pParam->name, "RssType") == 0)
    {
        pAdapter->HashType = (u8)pParam->val;
    }
    else if(strcmp((char*)pParam->name, "LinkCap") == 0)
    {
        pAdapter->LinkCapability = (u8)pParam->val;
        if(pAdapter->Mode == LOAD_MODE_NORMAL)
        {
            switch(pParam->val)
            {
            case LC_TYPE_10F:
            case LC_TYPE_100F:
                pAdapter->LinkMode = LINKMODE_NORMAL_100F;
                break;
            case LC_TYPE_1000F:
                pAdapter->LinkMode = LINKMODE_NORMAL_1000F;
                break;
            default:
                pAdapter->LinkMode = LINKMODE_NORMAL_100F;
            }
        }
        else if(pAdapter->Mode == LOAD_MODE_MAC_LOOPBACK)
        {
            switch(pParam->val)
            {
            case LC_TYPE_10F:
                pAdapter->LinkMode = LINKMODE_MACLP_10F;
                break;
            case LC_TYPE_100F:
                pAdapter->LinkMode = LINKMODE_MACLP_100F;
                break;
            case LC_TYPE_1000F:
                pAdapter->LinkMode = LINKMODE_MACLP_1000F;
                break;
            default:
                pAdapter->LinkMode = LINKMODE_MACLP_100F;
            }
        }
        else if(pAdapter->Mode == LOAD_MODE_PHY_LOOPBACK)
        {
            switch(pParam->val)
            {
            case LC_TYPE_10F:
                pAdapter->LinkMode = LINKMODE_PHYLP_10F;
                break;
            case LC_TYPE_100F:
                pAdapter->LinkMode = LINKMODE_PHYLP_100F;
                break;
            case LC_TYPE_1000F:
                pAdapter->LinkMode = LINKMODE_PHYLP_1000F;
                break;
            default:
                pAdapter->LinkMode = LINKMODE_PHYLP_100F;
            }
        }
    }
    else
    {
        printf("Don't know the parameter name.\n");
    }
}
        
int NicGetParam(PADAPTER pAdapter, char* sParamName, PVOID pOutBuf, u32 *pOutBufLen)
{
    int ret = STATUS_SUCCESSFUL;

    if (strcmp(sParamName, "MacAddr") == 0)
    {
        if(*pOutBufLen == 6)
        {
            u32     mac0 = 0, mac1 = 0;
            u32 reg = 0, value = 0;
            for (u8 index = 0; index < 39; index++) {
                if (!read_patch_from_efuse_per_index(gFuxiMemBase, gTarget, index, &reg, &value)) {
                    break; // reach the last item.
                }
                if (0x00 == reg) {
                    break; // reach the blank.
                }
                if (MACA0LR_FROM_EFUSE == reg) {
                    mac0 = value;
                }
                if (MACA0HR_FROM_EFUSE == reg) {
                    mac1 = value;
                }
            } 

            *(u32 *)(pAdapter->PermAddr) = mac0;
            *(u16 *)(pAdapter->PermAddr + 4) = (u16)mac1;

            memcpy(pOutBuf, pAdapter->PermAddr, 6);
        }
        else
        {
            *pOutBufLen = 6;
        }
    }
    else if (strcmp(sParamName, "VlanStrip") == 0)
    {
        if (*pOutBufLen != sizeof(bool))
        {
            ret = STATUS_FAILED;
        }
        else
        {
            *(bool*)pOutBuf = pAdapter->VlanStrip;
        }
    }
    else if (strcmp(sParamName, "MTU") == 0)
    {
        u32 val;
        if (*pOutBufLen != sizeof(u16))
        {
            ret = STATUS_FAILED;
        }
        else
        {
            val = 1514;
            *(u16*)pOutBuf = (u16)val - 4 - 4;   //pAdapter->MaxFrameSize;
        }
    }
    else if (strcmp(sParamName, "LldpChkAddr") == 0)
    {
        if (*pOutBufLen != sizeof(bool))
        {
            ret = STATUS_FAILED;
        }
        else
        {
            *(bool*)pOutBuf = pAdapter->lldpChkAddr;
        }
    }
    else if (strcmp(sParamName, "RxRingSize") == 0)
    {
        u32 val;
        if (*pOutBufLen != sizeof(u16))
        {
            ret = STATUS_FAILED;
        }
        else
        {
            *(u16*)pOutBuf = 256; //pAdapter->RxRingSize;
        }
    }
    else if (strcmp(sParamName, "RxBufBlockSize") == 0)
    {
        u32 val;
        if (*pOutBufLen != sizeof(u16))
        {
            ret = STATUS_FAILED;
        }
        else
        {
            *(u16*)pOutBuf = 1644; //pAdapter->RxBufBlockSize;
        }
    }
    else if (strcmp(sParamName, "DumpReg") == 0)
    {
//        ShowStatistics(pAdapter);
    }
    else if (strcmp(sParamName, "ClearStat") == 0)
    {
//        ClearStatistics(pAdapter);
    }

    if (ret != STATUS_SUCCESSFUL)
    {
        printf("Parameter error in NicGetParam()");
    }
    return ret;
}

u32 IoCtrl_Linux(
        PVOID               Driver,
        u32                 CtrlCode,
        PVOID               pInBuf,
        u32                 InBufLen,
        PVOID               pOutBuf,
        u32                 OutBufLen
        )
{
    PADAPTER pAdapter = (PADAPTER)Driver;
    int ret = 0;
    
    switch(CtrlCode)
    {
    case DEV_IOCTL_BIST:
        ret = test_bist(pAdapter);
        break;

    case DEV_IOCTL_REGVAL:
        ret = -1;
        //ret = test_registers(pAdapter);
        break;

    case DEV_IOCTL_SMBUS:
        ret = -1;
        //ret = test_smbus(pAdapter);
        break;

    case DEV_IOCTL_FLASH:
        break;

    case DEV_IOCTL_EFUSE:
        break;

    case DEV_IOCTL_MEM_REG:
        ret = RegViaMem(pAdapter, (PRW_REG)pInBuf, InBufLen);
        break;

    case DEV_IOCTL_IO_REG:
        ret = RegViaIo(pAdapter, (PRW_REG)pInBuf, InBufLen);
        break;

    case DEV_IOCTL_CFG_REG:
        {
            PRW_REG pRwReg = (PRW_REG)pInBuf;
            if (InBufLen != sizeof(*pRwReg))
            {
                printf("Input parameter error\n");
                return STATUS_FAILED;
            }

            if(pRwReg->bRead)
            {
                CFG_R32(pAdapter, pRwReg->reg, &pRwReg->val);
            }
            else
            {
                CFG_W32(pAdapter, pRwReg->reg, pRwReg->val);
            }
        }
        break;

    case DEV_IOCTL_PHYREG:
        {
            PRW_REG pRwReg = (PRW_REG)pInBuf;
            if(pRwReg->bRead)
            {
                PHY_R(pAdapter, pRwReg->reg, (u16*)&pRwReg->val);
            }
            else
            {
                PHY_W(pAdapter, pRwReg->reg, (u16)pRwReg->val);
            }
        }
        break;

    case DEV_IOCTL_PHYDBG:
        {
            PRW_REG pRwReg = (PRW_REG)pInBuf;
            if(pRwReg->bRead)
            {
                PHY_W(pAdapter, L1F_MII_DBG_ADDR, pRwReg->reg);
                PHY_R(pAdapter, L1F_MII_DBG_DATA, (u16*)&pRwReg->val);
            }
            else
            {
                PHY_W(pAdapter, L1F_MII_DBG_ADDR, pRwReg->reg);
                PHY_W(pAdapter, L1F_MII_DBG_DATA, (u16)pRwReg->val);
            }
        }
        break;

    case DEV_IOCTL_PHY_EXTREG:
        {
            PRW_REG pRwReg = (PRW_REG)pInBuf;
            if(pRwReg->bRead)
            {
                PHYEXT_R(pAdapter, pRwReg->dev, pRwReg->reg, (u16*)&pRwReg->val);
            }
            else
            {
                PHYEXT_W(pAdapter, pRwReg->dev, pRwReg->reg, (u16)pRwReg->val);
            }
        }
        break;

    case DEV_IOCTL_SET_RSS_TBL:
        ret = -1;
        //memcpy(pAdapter->IdtTable, pInBuf, InBufLen);
        //pAdapter->IdtSize = (u16)InBufLen;
        //NicRssIdt(pAdapter, (u8*)pInBuf, (u16)InBufLen);
        break;

    case DEV_IOCTL_SET_RSS_KEY:
        ret = -1;
        //memcpy(pAdapter->HashSecretKey, pInBuf, InBufLen);
        //pAdapter->HashKeySize = (u16)InBufLen;
        //NicRssKey(pAdapter, (u8*)pInBuf, (u16)InBufLen);
        break;
    case DEV_IOCTL_SET_RSS_BCPU:
        pAdapter->BaseCpuNum = *(u16*)pInBuf;
        break;

    case DEV_IOCTL_SET_PARAM:
        if (InBufLen != sizeof(SET_DEV_PARAM))
        {
            ret = STATUS_FAILED;
        }
        else
        {
            NicSetParam(pAdapter, (PSET_DEV_PARAM)pInBuf);
        }
        break;

    case DEV_IOCTL_GET_PARAM:
        ret = NicGetParam(pAdapter, (char*)pInBuf, pOutBuf, &OutBufLen);
        break;
    }
    return ret;
}


PVOID EnableInterrupt_Linux(
        PVOID               Driver,
        bool                bEnable
        )
{
    Driver = Driver;
    bEnable = bEnable;
    return 0;
}

