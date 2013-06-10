########################################################################################
# Initialization file for 8309 SOM
########################################################################################

variable MBAR 	0xFF400000

proc IMMR {reg_off} {
	global MBAR
	
	return p:0x[format %x [expr {$MBAR + $reg_off}]]
}

proc 8309SOM_init_core {} {

	global MBAR

	variable SSP_GROUP "Standard Special Purpose Registers/"
	variable GPR_GROUP "General Purpose Registers/"
	variable G2_LE_GROUP "G2_LE Core Special Purpose Registers/"

	#setMMRBaseAddr 0xFF400000
	reg ${G2_LE_GROUP}MBAR = 0xFF400000 	

	# change internal MMR base from 0xff400000 (reset value) to 0xe0000000
	# IMMRBAR = 0xe0000000
	mem [IMMR 0x0] = 0xe0000000
	set MBAR 0xe0000000

	#setMMRBaseAddr 0xe0000000
	reg ${G2_LE_GROUP}MBAR = 0xe0000000


	##############################################
	# System Configuration - Local Access Windows 
	##############################################

	# Local Bus Local Access Windows
	#################################

	# WINDOW 0 - NOR FLASH

	# LBLAWBAR0  - begining at 0xff800000 
	mem [IMMR 0x20] = 0xff800000
	# LBLAWAR0   - enable, size = 8MB 
	mem [IMMR 0x24] = 0x80000016 


	# WINDOW 1 - NAND Flash 

	# LBLAWBAR1  - begining at 0xf0000000
	mem [IMMR 0x28] = 0xf0000000
	# LBLAWAR1   - enable, size = 32kB
	mem [IMMR 0x2C] = 0x8000000e 

	# PCI Local Access Windows
	#################################
	# WINDOW 0

	# PCILAWBAR0 - begining at 0x80000000
	mem [IMMR 0x60] = 0x80000000
	# PCILAWAR0  - enable, size = 512MB
	mem [IMMR 0x64] = 0x8000001c


	# DDR Local Access Windows
	#################################
	# WINDOW 0 - 1st DDR SODIMM

	# DDRLAWBAR0 - begining at 0x00000000
	mem [IMMR 0xA0] = 0x00000000 
	# DDRLAWAR0  - enable, size = 256MB
	mem [IMMR 0xA4] = 0x8000001b 


	#*********************************
	# DDR2 Controller Registers
	#*********************************

	#DDRCDR
	mem [IMMR 0x128] = 0x73000002


	# DDR_SDRAM_CLK_CNTL
	# CLK_ADJST = b'010' ; 2 Clocks
	mem [IMMR 0x2130] = 0x02000000

	# CS0_BNDS
	# SA0 = b'000000000000'
	# EA0 = b'000000001111'
	# 256MB
	mem [IMMR 0x2000] = 0x0000000f

	# CS0_CONFIG
	# CS_0_EN = b'1'
	# AP_0_EN = b'1'
	# ODT_RD_CFG = b'0'
	# ODT_WR_CFG = b'1'
	# BA_BITS_CS_0 = b'00'
	# ROW_BITS_CS_0 = b'001' ; 13 row bits
	# COL_BITS_CS_0 = b'010' ; 10 columns bits
	mem [IMMR 0x2080] = 0x80840102

	# TIMING_CFG_3
	# EXT_REFREC = b'000' ; 0 Clocks
	mem [IMMR 0x2100] = 0x00000000
		

	# TIMING_CONFIG_1
	# bit 1-3 = 2 - PRETOACT precharge activate interval 2 clock cycles
	# bit 4-7 = 6 - ACTTOPRE activate to precharge interval 6 clock cycles
	# bit 9-11 = 2 = ACTTORW activate to r/w interval 2 clock cycles 
	# bit 13 - 15 = 5 - CASLAT CAS latency 3 clock cycles
	# bit 16 - 19 = 6 - REFREC refresh recovery time 14 clock cycles 
	# bit 21 - 23 = 2 - WRREC data to precharge interval 2 clock cycles 
	# bit 25 - 27 = 2 - ACTTOACT activate to activate interval 2 clock cycles
	# bit 29 - 31 = 2 - WRTORD write data to read command interval 2 clock cycles
	mem [IMMR 0x2108] = 0x26256222

	# TIMING_CONFIG_2
	# bit 19-21  = b010  - WR_DATA_DELAY - 1/2 DRAM clock delay
	mem [IMMR 0x210C] = 0x0f9028c7


	# TIMING_CFG_0
	# RWT = b'00' ; 0 Clocks
	# WRT = b'00' ; 0 Clocks
	# RRT = b'00' ; 0 Clocks
	# WWT = b'00' ; 0 Clocks
	# ACT_PD_EXIT = b'010' ; 2 Clocks
	# PRE_PD_EXIT = b'010' ; 2 Clocks
	# ODT_PD_EXIT = b'1000' ; 8 Clocks
	# MRS_CYC = b'0010' ; 2 Clocks
	mem [IMMR 0x2104] = 0x00220802

	# DDR_SDRAM_CFG
	# MEM_EN = b'0'
	# SREN = b'1'
	# RD_EN = b'0'
	# SDRAM_TYPE = b'011'
	# DYN_PWR = b'0'
	# 32_BE = b'1'
	# 8_BE = b'0'
	# NCAP = b'0'
	# 2T_EN = b'0'
	# x32_EN = b'0'
	# PCHB8 = b'0'
	# HSE = b'0'
	# MEM_HALT = b'0'
	# BI = b'0'
	mem [IMMR 0x2110] = 0x43080000

	# DDR_SDRAM_CFG_2
	# FRC_SR = b'0'
	# DQS_CFG = b'00'
	# ODT_CFG = b'10'
	# NUM_PR = b'0001'
	# D_INIT = b'0'
	mem [IMMR 0x2114] = 0x00401000

	# DDR_SDRAM_MODE
	# Extended Mode Register: Outputs=0 or 1?
	# Mode Register
	mem [IMMR 0x2118] = 0x44400232 

	# DDR_SDRAM_MODE_2
	# Extended Mode Register 2
	# Extended Mode Register 3
	mem [IMMR 0x211C] = 0x8000c000
		
	# DDR_SDRAM_INTERVAL
	# REFINT = 800 Clocks
	# BSTOPRE = 100 Clocks
	mem [IMMR 0x2124] = 0x03200064

	#delay before enable
	wait 300

	#Enable: DDR_SDRAM_CFG
	mem [IMMR 0x2110] = 0xc3080000
		
	#wait for DRAM data initialization
	wait 300


	##############################################
	# Local Bus Interface (LBIU) Configuration
	##############################################

	# CS0 - 8MB NOR FLASH
	# BR0 base address at 0xff800000, port size 16 bit, GPCM, valid
	mem [IMMR 0x5000] = 0xff801001  
	# OR0 8MB flash size, 15 w.s., timing relaxed
	#writemem.l 0xe0005004 0xff806ff7
	mem [IMMR 0x5004] = 0xff806ff7

	# CS1 - NAND Flash 32 KB
	# BR1 base address at 0xF0000000, port size 8 bit, FCM, valid
	mem [IMMR 0x5008] = 0xf0000c21 
	# OR1 32KB size
	#writemem.l 0xe000500c 0xFFFF8796 
	mem [IMMR 0x500C] = 0xFFFF8796 


	# LBCR - local bus enable
	mem [IMMR 0x50D0] =  0x00000000

	# LCRR
	# bit 14 - 15 = 0b11 - EADC - 3 external address delay cycles 
	# bit 28 - 31 = 0x0010  - CLKDIV - system clock:memory bus clock = 2
	mem [IMMR 0x50D4] =  0x00030002


	# FP available, machine check disable, exception vectors at 0x0000_0000
	reg ${SSP_GROUP}MSR = 0x2000 

	# ACR - Enable Core
	mem [IMMR 0x800] = 0x00000000


	#
	# NAND Flash settings
	# FMR
	mem [IMMR 0x50E0] = 0x0000E000 


	# MRTPR - refresh timer prescaler
	mem [IMMR 0x5084] = 0x20000000 


	# CAN_DBG_CTRL - contains the bits to control the FlexCAN module
	mem [IMMR 0x148] = 0x40404040

	reg ${GPR_GROUP}SP = 0xf
}

proc envsetup {} {
	# Environment Setup
	radix x 
	config hexprefix 0x
	config MemIdentifier v
	config MemWidth 32 
	config MemAccess 32 
	config MemSwap off
}

#-------------------------------------------------------------------------------
# Main                                                                          
#-------------------------------------------------------------------------------

envsetup

8309SOM_init_core