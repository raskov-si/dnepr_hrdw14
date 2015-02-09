/*
 * File:    mii.h
 * Purpose:     
 *
 * Notes:
 */

#ifndef _T8_5282_H_
#define _T8_5282_H_

#ifdef	__cplusplus
    extern "C" {
#endif

/********************************************************************/

/* �������� �������� ������� MII */
// ������ ���� <=  2.5 ���  a - �������� ������� MII � ���      
/*        
      FEC_MDC_KHZ = (FEC_MDC_KHZ <= 2500u) ? FEC_MDC_KHZ : 2500u         
      FEC_MDC_KHZ = SYSTEM_CLOCK_KHZ / (MII_SPEED x 2)        
      SYSTEM_CLOCK_KHZ / FEC_MDC_KHZ  = (MII_SPEED x 2)                
      MII_SPEED = SYSTEM_CLOCK_KHZ / (FEC_MDC_KHZ * 2)
*/      
#define FEC_MII_CLOCK_DEV_CALC(a)  ( SYSTEM_CLOCK_KHZ / ( ((a <= 2500u) ? a : 2500u) * 2) )
              
/* MII Speed Settings */
#define FEC_MII_10BASE_T        0
#define FEC_MII_100BASE_TX      1

/* MII Duplex Settings */
#define FEC_MII_HALF_DUPLEX     0
#define FEC_MII_FULL_DUPLEX     1

/* Timeout for MII communications */
#define FEC_MII_TIMEOUT         0x10000

/* External Interface Modes */
#define FEC_MODE_7WIRE          0
#define FEC_MODE_MII            1
#define FEC_MODE_LOOPBACK       2   /* Internal Loopback */
      
 /* Status bits in buffer descriptors */
#define MCF_FEC_TxBD_R                  0x8000
#define MCF_FEC_TxBD_INUSE              0x4000
#define MCF_FEC_TxBD_TO1                0x4000
#define MCF_FEC_TxBD_W                  0x2000
#define MCF_FEC_TxBD_TO2                0x1000
#define MCF_FEC_TxBD_L                  0x0800
#define MCF_FEC_TxBD_TC                 0x0400
#define MCF_FEC_TxBD_DEF                0x0200
#define MCF_FEC_TxBD_HB                 0x0100
#define MCF_FEC_TxBD_LC                 0x0080
#define MCF_FEC_TxBD_RL                 0x0040
#define MCF_FEC_TxBD_UN                 0x0002
#define MCF_FEC_TxBD_CSL                0x0001

#define MCF_FEC_RxBD_E                  0x8000
#define MCF_FEC_RxBD_INUSE              0x4000
#define MCF_FEC_RxBD_R01                0x4000
#define MCF_FEC_RxBD_W                  0x2000
#define MCF_FEC_RxBD_R02                0x1000
#define MCF_FEC_RxBD_L                  0x0800
#define MCF_FEC_RxBD_M                  0x0100
#define MCF_FEC_RxBD_BC                 0x0080
#define MCF_FEC_RxBD_MC                 0x0040
#define MCF_FEC_RxBD_LG                 0x0020
#define MCF_FEC_RxBD_NO                 0x0010
#define MCF_FEC_RxBD_CR                 0x0004
#define MCF_FEC_RxBD_OV                 0x0002
#define MCF_FEC_RxBD_TR                 0x0001
      

/*=============================================================================================================*/

/*! \brief ���������� ������ FEC'�  */
#pragma pack(push)
#pragma pack(1)
typedef struct BufferDescriptor 
{
   volatile u16  cstatus;     /*!< control and status */
   volatile u16  length;      /*!< transfer length    */
   volatile u8 * addr;        /*!< buffer address     */
} t_txrx_desc;
#pragma pack(pop)
      
/*! \brief ������������ FEC'�  */
typedef struct _FEC_CONFIG
{
    u32         fec_mii_speed;                  /*!< �������� ������� ��� MII, ��������� ����� FEC_MII_CLOCK_DEV_CALC               */
    u8          fec_mode;                       /*!< ����� FEC, FEC_MODE_7WIRE, FEC_MODE_MII, FEC_MODE_LOOPBACK                     */
    u8          mac_addr[6];		        /*!< ���-�����                                                                      */
    u8          ignore_mac_adress_when_recv;    /*!< Promiscuous mode. ��� ethernet ������ ����������� ���� ��� ������������ ������ */
    u8          rcv_broadcast_clock;	        /*!< RX ������ ����������� �� broadcast ������ ��� ���                              */
    u16         fec_max_eth_pocket;
    t_txrx_desc *rxbd_ring;
    u8          rxbd_ring_len;
    t_txrx_desc *txbd_ring;
    u8          txbd_ring_len;
} t_fec_config;
            
/*=============================================================================================================*/
      
//Fucntion Protoypes
void t8_m5282_fec_init(t_fec_config *input);

u32  t8_m5282_fec_mdio_write(u32, u32, u16);
u32  t8_m5282_fec_mdio_read(u32, u32, u16*);


//void fec_mii_reg_printf(void);

/********************************************************************/
//Register Mask and Other
//===============
/* Definition of allowed values for MDCSEL */
#define MII_MDCSEL(x) x/5000000

#define MII_WRITE   0x01
#define MII_READ    0x02

#define TCMD_START 0x01 	/* Transmit buffer frame */
#define TCMD_PAUSE 0x02 	/* Transmit PAUSE frame */
#define TCMD_ABORT 0x03 	/* Abort transmission */

/* PHY registers symbolic names */
/* (located in MII memory map, accessible through MDIO) */
#define PHY_REG_CR      0x00 /* Control Register */
#define PHY_REG_SR      0x01 /* Status Register */
#define PHY_REG_ID1     0x02 /* PHY Identification Register 1 */
#define PHY_REG_ID2     0x03 /* PHY Identification Register 2 */
#define PHY_REG_ANAR    0x04 /* Auto-Negotiation Advertisement Register */
#define PHY_REG_ANLPAR  0x05 /* Auto-Negotiation Link Partner Ability Register */
#define PHY_REG_ER      0x06 /* Auto-Negotiation Expansion Register */
#define PHY_REG_NPTR    0x07 /* Auto-Negotiation Next Page Transfer Register */
#define PHY_REG_IR      0x10 /* Interrupt Register */
#define PHY_REG_PSR     0x11 /* Proprietary Status Register */
#define PHY_REG_PCR     0x12 /* Proprietary Control Register */
#define PHY_REG_10BTBC  0x13 /* 10Base-T Bypass Control Register */
#define PHY_REG_100BXBC 0x14 /* 100Base-X Bypass Control Register */
#define PHY_REG_ADDR    0x15 /* Test & Trim Control Register */
#define PHY_REG_DSPRC   0x17 /* DSP Reset Control */
#define PHY_REG_DSPRR1  0x18 /* 100Base-X DSP Read Registers */
#define PHY_REG_DSPRR2  0x19
#define PHY_REG_DSPRR3  0x1A
#define PHY_REG_DSPWR1  0x1B /* 100Base-X DSP Write Registers */
#define PHY_REG_DSPWR2  0x1C
#define PHY_REG_DSPWR3  0x1D
#define PHY_REG_TxCR	0x1F	//FSL added

/* PHY registers structure */
/* 0 - Control Register */
#define PHY_R0_RESET    0x8000  /* Reset */
#define PHY_R0_LB       0x4000  /* Loop Back */
#define PHY_R0_DR       0x2000  /* Data Rate (100Mb/s) */
#define PHY_R0_ANE      0x1000  /* Auto-Negotiation Enable */
#define PHY_R0_PD       0x0800  /* Power Down */
#define PHY_R0_ISOLATE  0x0400  /* Isolate (MII is disconnected) */
#define PHY_R0_RAN      0x0200  /* Restart Auto-Negotiation */
#define PHY_R0_DPLX     0x0100  /* Duplex (Full duplex) */
#define PHY_R0_CT       0x0080  /* Collision Test (Enable) */
#define PHY_R0_DT       0x0001  /* Disable Transmitter */

/* 1 - Status Register */
#define PHY_R1_100T4    0x8000  /* 100BASET4 Supported */
#define PHY_R1_100F     0x4000  /* 100Mb/s Full Duplex Supported */
#define PHY_R1_100H     0x2000  /* 100Mb/s Half Duplex Supported */
#define PHY_R1_10F      0x1000  /* 10Mb/s Full Duplex Supported */
#define PHY_R1_10H      0x0800  /* 10Mb/s Half Duplex Supported */
#define PHY_R1_SUP      0x0040  /* MI Preamble Supression (capable of) */
#define PHY_R1_ANC      0x0020  /* Auto Negotiation Complete */
#define PHY_R1_RF       0x0010  /* Remote Fault */
#define PHY_R1_ANA      0x0008  /* Auto-Negotiation Ability (present) */
#define PHY_R1_LS       0x0004  /* Link Status (Link is Up) */
#define PHY_R1_JD       0x0002  /* Jabber Detect (detected) */
#define PHY_R1_EC       0x0001  /* Extended Capability (regs 2 to 31 exists) */

/* 2 - PHY Identifier Register 1 */
/* 3 - PHY Identifier Register 2 */
/* read only - contains Manufacturer's info etc.
   see documentation for the detailed description */

/* 4 - Auto Negotiation Advertisement Register */
#define PHY_R4_NP       0x8000  /* Next Page (capable of sending next pages) */
#define PHY_R4_RF       0x2000  /* Remote Fault */
#define PHY_R4_FC       0x0400  /* Flow Control */
#define PHY_R4_100F     0x0100  /* 100Base-TX Full Duplex Capable */
#define PHY_R4_100H     0x0080  /* 100Base-TX Half Duplex Capable */
#define PHY_R4_10F      0x0040  /* 10Base-T Full Duplex Capable */
#define PHY_R4_10H      0x0020  /* 10Base-T Half Duplex Capable */
/* bits 4 to 0 are Selector Field (IEEE Std 802.3 = 00001) */

/* 5 - Auto Negotiation Link Partner Ability Register (Base Page & Next Page) */
/* read only - please consult PHY documentation */
#define PHY_R5_FCTL      0x0400  /* 10Base-T Half Duplex Capable */

/* 16 - Interrupt Control Register */
#define PHY_R16_ACKIE 	0x4000	//Acknowledge Bit Received Interrupt Enable
#define PHY_R16_PRIE 	0x2000	//Page Received INT Enable
#define PHY_R16_LCIE 	0x1000	//Link Changed Enable
#define PHY_R16_ANIE 	0x0800	//Auto-Negotiation Changed Enable
#define PHY_R16_PDFIE 	0x0400	//Parallel Detect Fault Enable
#define PHY_R16_RFIE 	0x0200	//Remote Fault Interrupt Enable
#define PHY_R16_JABIE	0x0100	//Jabber Interrupt Enable

#define PHY_R16_ACKR    0x0040	//Acknowledge Bit Received Interrupt
#define PHY_R16_PGR 	0x0020	//Page Received 
#define PHY_R16_LKC 	0x0010	//Link Changed 
#define PHY_R16_ANC 	0x0008	//Auto-Negotiation Changed 
#define PHY_R16_PDF 	0x0004	//Parallel Detect Fault
#define PHY_R16_RMTF 	0x0002  //Remote Fault Interrupt
#define PHY_R16_JABI	0x0001	//Jabber Interrupt

////Proprietary Status Register
#define PHY_R17_LNK   0x4000	//
#define PHY_R17_DPM   0x2000	//Duplex Mode
#define PHY_R17_SPD   0x1000	//Speed
#define PHY_R17_ANNC  0x0400	//Auto-Negotiation Complete
#define PHY_R17_PRCVD 0x0200	//
#define PHY_R17_ANCM  0x0100	// Auto-Negotiation (A-N) Common Operating Mode
#define PHY_R17_PLR   0x0020	//

/********************************************************************/

#ifdef	__cplusplus
    }
#endif

#endif /* _T8_5282_H_ */
