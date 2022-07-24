#include <stdio.h>
#include <stdlib.h>
#include <sdcard/gcsd.h>
#include <string.h>
#include <malloc.h>
#include <unistd.h>
#include <ogc/lwp_watchdog.h>
#include <fcntl.h>
#include <ogc/system.h>
#include "ffshim.h"
#include "fatfs/ff.h"

#include "stub.h"
#define STUB_ADDR  0x80001000
#define STUB_STACK 0x80003000

u8 *dol = NULL;
size_t dol_size = 0;
//u8 *ipl = NULL;
char *path = "/KUNAIGC/recovery.dol";

#define POLYNOMIAL 0x1021  /* 11011 followed by 0's */
#define INITIAL_REMAINDER 0x1D0F
/*
 * The width of the CRC calculation and result.
 * Modify the typedef for a 16 or 32-bit CRC standard.
 */
typedef uint16_t crc;

crc ipl_crc = 0;

#define WIDTH  (8 * sizeof(crc))
#define TOPBIT (1 << (WIDTH - 1))
crc  crcTable[256];

void crcInit(void)
{
    crc  remainder;


    /*
     * Compute the remainder of each possible dividend.
     */
    for (int dividend = 0; dividend < 256; ++dividend)
    {
        /*
         * Start with the dividend followed by zeros.
         */
        remainder = dividend << (WIDTH - 8);

        /*
         * Perform modulo-2 division, a bit at a time.
         */
        for (uint8_t bit = 8; bit > 0; --bit)
        {
            /*
             * Try to divide the current data bit.
             */
            if (remainder & TOPBIT)
            {
                remainder = (remainder << 1) ^ POLYNOMIAL;
            }
            else
            {
                remainder = (remainder << 1);
            }
        }

        /*
         * Store the result into the table.
         */
        crcTable[dividend] = remainder;
    }

}   /* crcInit() */

crc crcFast(uint8_t const message[], int nBytes)
{
    uint8_t data;
    crc remainder = INITIAL_REMAINDER;

    /*
     * Divide the message by the polynomial, a byte at a time.
     */
    for (int byte = 0; byte < nBytes; ++byte)
    {
        data = message[byte] ^ (remainder >> (WIDTH - 8));
        remainder = crcTable[data] ^ (remainder << 8);
    }

    /*
     * The final remainder is the CRC.
     */
    return (remainder);

}   /* crcFast() */

void dol_alloc(int size)
{
    int mram_size = (SYS_GetArenaHi() - SYS_GetArenaLo());
    kprintf("Memory available: %iB\n", mram_size);

    kprintf("DOL size is %iB\n", size);

    if (size <= 0)
    {
        kprintf("Empty DOL\n");
        return;
    }

    dol = (u8 *) memalign(32, size);

    if (!dol)
    {
        kprintf("Couldn't allocate memory\n");
    }
}

//void ipl_alloc(int size)
//{
//    int mram_size = (SYS_GetArenaHi() - SYS_GetArenaLo());
//    kprintf("Memory available: %iB\n", mram_size);
//
//    kprintf("IPL size is %iB\n", size);
//
//    if (size <= 0)
//    {
//        kprintf("Empty IPL\n");
//        return;
//    }
//
//    ipl = (u8 *) memalign(32, size);
//
//    if (!ipl)
//    {
//        kprintf("Couldn't allocate memory\n");
//    }
//}

int load_fat(const char *slot_name, const DISC_INTERFACE *iface_)
{
    int res = 1;

    kprintf("Trying %s\n", slot_name);

    FATFS fs;
    iface = iface_;
    if (f_mount(&fs, "", 1) != FR_OK)
    {
        kprintf("Couldn't mount %s\n", slot_name);
        res = 0;
        goto end;
    }

    char name[256];
    f_getlabel(slot_name, name, NULL);
    kprintf("Mounted %s as %s\n", name, slot_name);

    kprintf("Reading %s\n", path);
    FIL file;
    if (f_open(&file, path, FA_READ) != FR_OK)
    {
        kprintf("Failed to open file\n");
        res = 0;
        goto unmount;
    }

    dol_size = f_size(&file);
    dol_alloc(dol_size);
    if (!dol)
    {
        res = 0;
        goto unmount;
    }
    UINT _;
    f_read(&file, dol, dol_size, &_);
    f_close(&file);

unmount:
    kprintf("Unmounting %s\n", slot_name);
    iface->shutdown();
    iface = NULL;

end:
    return res;
}

#define PC_READY 0x80
#define PC_OK    0x81
#define GC_READY 0x88
#define GC_OK    0x89

extern u8 __xfb[];

#define KUNAI_LOADER_SIZE_ADDR 	0x023000UL
#define KUNAI_LOADER_ADDR 		0x024000UL	//max. recovery payload size: 141.280 bytes (147.456 - 6176)

int kunai_load_internal(void){
	kprintf("Trying loading from internal Memory\n\n");
	int res = 1;
	u32 addr = (KUNAI_LOADER_ADDR << 6); //convert address to EXI scheme
	u32 size_addr = (KUNAI_LOADER_SIZE_ADDR << 6); //convert address to EXI scheme

	EXI_Lock(EXI_CHANNEL_0, EXI_DEVICE_1, NULL);
	EXI_Select(EXI_CHANNEL_0, EXI_DEVICE_1, EXI_SPEED16MHZ);
	EXI_Imm(EXI_CHANNEL_0, &size_addr, 4, EXI_WRITE, NULL);
	EXI_Sync(EXI_CHANNEL_0);
	EXI_Imm(EXI_CHANNEL_0, &dol_size, 4, EXI_READ, NULL);
	EXI_Sync(EXI_CHANNEL_0);
	EXI_Imm(EXI_CHANNEL_0, &ipl_crc, 2, EXI_READ, NULL);
	EXI_Sync(EXI_CHANNEL_0);

	dol_alloc(dol_size);
	if(!dol)
	{
		res = 0;
		goto end;
	}

	EXI_Deselect(EXI_CHANNEL_0);
	EXI_Unlock(EXI_CHANNEL_0);

	EXI_Lock(EXI_CHANNEL_0, EXI_DEVICE_1, NULL);
	EXI_Select(EXI_CHANNEL_0, EXI_DEVICE_1, EXI_SPEED16MHZ);
	EXI_Imm(EXI_CHANNEL_0, &addr, 4, EXI_WRITE, NULL);
	EXI_Sync(EXI_CHANNEL_0);

	kprintf("Receiving file...\n");
	u32 * pointer = (u32 *) dol;
	u16 loops = (dol_size + 2) / 4;
	while(loops--){
		EXI_Imm(EXI_CHANNEL_0, pointer++, 4, EXI_READ, NULL);
		EXI_Sync(EXI_CHANNEL_0);
	}

end:
	EXI_Deselect(EXI_CHANNEL_0);
	EXI_Unlock(EXI_CHANNEL_0);

	crcInit();
	crc dol_crc = crcFast(dol, (int) dol_size);
	if(ipl_crc != dol_crc) {
		kprintf("CRC-check failed!\n");
		kprintf("\texpected   crc: %04X\n", ipl_crc);
		kprintf("\tcalculated crc: %04X\n", dol_crc);
		dol = NULL;
		res = 0;
	}

	return res;
}

void kunai_disable(void) {
	u32 addr = 0xFFFFFFFF; //disable the KunaiGC by sending only '1' to the Bus
	EXI_Lock(EXI_CHANNEL_0, EXI_DEVICE_1, NULL);
	EXI_Select(EXI_CHANNEL_0, EXI_DEVICE_1, EXI_SPEED8MHZ);
	EXI_Imm(EXI_CHANNEL_0, &addr, 4, EXI_WRITE, NULL);
	EXI_Sync(EXI_CHANNEL_0);
	EXI_Deselect(EXI_CHANNEL_0);
	EXI_Unlock(EXI_CHANNEL_0);
}

int main()
{
	VIDEO_Init ();		/*** ALWAYS CALL FIRST IN ANY LIBOGC PROJECT!
						     Not only does it initialise the video
						     subsystem, but also sets up the ogc os ***/

		PAD_Init ();			/*** Initialise pads for input ***/
		// get default video mode
		GXRModeObj *rmode = VIDEO_GetPreferredMode(NULL);

		switch (rmode->viTVMode >> 2)
		{
		case VI_PAL:
			// 576 lines (PAL 50Hz)
			// display should be centered vertically (borders)
			//Make all video modes the same size so menus doesn't screw up
			rmode = &TVPal576IntDfScale;
			rmode->xfbHeight = 480;
			rmode->viYOrigin = (VI_MAX_HEIGHT_PAL - 480)/2;
			rmode->viHeight = 480;
			break;
		default:
			break;
		}

		if(VIDEO_HaveComponentCable()) rmode = &TVNtsc480Prog;
		VIDEO_Configure (rmode);
		VIDEO_ClearFrameBuffer (rmode, __xfb, COLOR_BLACK);
		VIDEO_SetNextFramebuffer (__xfb);
		VIDEO_SetBlack (0);
		VIDEO_Flush ();

		VIDEO_WaitVSync ();		/*** Wait for VBL ***/
		if (rmode->viTVMode & VI_NON_INTERLACE)
			VIDEO_WaitVSync ();
    CON_Init(__xfb, 0, 0, rmode->fbWidth, rmode->xfbHeight, rmode->fbWidth * VI_DISPLAY_PIX_SZ);

    PAD_ScanPads();

    kprintf("\n\nKunaiRecovery - based on IPLboot\n");

    //only scan for recovery file if 'Z' is held
    if(PAD_ButtonsHeld(PAD_CHAN0) & PAD_TRIGGER_Z) {

    	if (load_fat("sdb", &__io_gcsdb)) goto load;

    	if (load_fat("sda", &__io_gcsda)) goto load;

    	if (load_fat("sd2", &__io_gcsd2)) goto load;
    }

    if (kunai_load_internal()) goto load;

//    u16 err_ptr = 0;
//    for(u16 i = 0; i < 0x7020; i++) {
//    	if(((u32 *) ipl)[i] != ((u32 *) dol)[i]) {
//    		err_ptr = i;
//    		break;
//    	}
//    }


load:

	PAD_ScanPads();
	while (PAD_ButtonsHeld(PAD_CHAN0) & PAD_BUTTON_DOWN)
	{
		VIDEO_WaitVSync();
		PAD_ScanPads();
	}

	if (dol)
	{
		memcpy((void *) STUB_ADDR, stub, stub_size);
		DCStoreRange((void *) STUB_ADDR, stub_size);

		SYS_ResetSystem(SYS_SHUTDOWN, 0, FALSE);
		SYS_SwitchFiber((intptr_t) dol, 0,
				(intptr_t) NULL, 0,
				STUB_ADDR, STUB_STACK);
	}

    kunai_disable();
    // If we reach here, all attempts to load a DOL failed
    // Since we've disabled the Qoob, we wil reboot to the Nintendo IPL
    return 0;
}
