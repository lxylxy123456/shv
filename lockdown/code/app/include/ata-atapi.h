//------------------------------------------------------------------------------
// ata/atapi protocol/command definitions
// author: amit vasudevan (amitvasudevan@acm.org)
#ifndef __ATA_ATAPI_H_
#define __ATA_ATAPI_H_

#define LBA28					1
#define LBA48					2
#define LBANONE				0

#define LBA28MAXSECTOR	0x0fffffffUL

// the default sector size
#define ATA_SECTOR_SIZE  512
 
// default IRQ numbers of the ATA controllers 
#define ATA_IRQ_PRIMARY     0x0E
#define ATA_IRQ_SECONDARY   0x0F
 
// ATA I/O ports, indexed by "bus"
#define ATA_DATA(x)         (x)
#define ATA_FEATURES(x)     (x+1)
#define ATA_SECTOR_COUNT(x) (x+2)
#define ATA_LBALOW(x)     	(x+3)
#define ATA_LBAMID(x)     	(x+4)
#define ATA_LBAHIGH(x)     	(x+5)
#define ATA_DRIVE_SELECT(x) (x+6)
#define ATA_COMMAND(x)      (x+7)
#define ATA_DCR(x)          (x+0x206)   // device control register 
 
// valid values for "bus" 
//#define ATA_BUS_PRIMARY     0x1F0   //generic
//#define ATA_BUS_PRIMARY			0xC000
//#define ATA_BUS_SECONDARY   0x170
#define ATA_BUS_PRIMARY      (LDN_IDE_BUS)

// valid values for "drive" 
#define ATA_DRIVE_MASTER    0xA0
#define ATA_DRIVE_SLAVE     0xB0

// ATA command codes
#define CMD_READ_DMA				0xC8
#define CMD_WRITE_DMA				0xCA
#define CMD_READ_DMA_EXT		0x25
#define CMD_WRITE_DMA_EXT		0x35

#define CMD_READ_MULTIPLE		0xC4
#define CMD_WRITE_MULTIPLE	0xC3
#define CMD_READ_SECTORS		0x20
#define CMD_WRITE_SECTORS		0x30

#define CMD_READ_MULTIPLE_EXT	0x29
#define CMD_WRITE_MULTIPLE_EXT	0x39
#define CMD_READ_SECTORS_EXT		0x24
#define CMD_WRITE_SECTORS_EXT		0x34

#define	CMD_IDENTIFY_PACKET_DEVICE	0xA1

/*
//some defines to construct 32/64 bit numbers for 28bit and 48bit LBA addressing
#define LBA28BIT_TO_32BITVAL(bits27_24, bits23_16, bits15_8, bits7_0) \
	( (u32) ( ((u32)bits27_24 << 24) | ((u32)bits23_16 << 16) | ((u32)bits15_8 << 8) | (u32)bits7_0 ) )

#define LBA48BIT_TO_64BITVAL(bits63_56, bits55_48, bits47_40, bits39_32, bits31_24, bits23_16, bits15_8, bits7_0) \
		( (u64) ( \
			((u64)bits63_56 << 56) | ((u64)bits55_48 << 48) | ((u64)bits47_40 << 40) | \
			((u64)bits39_32 << 32) | ((u64)bits31_24 << 24) | ((u64)bits23_16 << 16) | \
			((u64)bits15_8 << 8) | (u64)bits7_0 \
		) )
*/

#ifndef __ASSEMBLY__


#endif

#endif /* __ATA_ATAPI_H_ */
