#include <stdio.h>
#include <stdlib.h>
#include <sdcard/gcsd.h>
#include <string.h>
#include <malloc.h>
#include <unistd.h>
#include <ogc/lwp_watchdog.h>
#include <fcntl.h>
#include <ogc/system.h>
#include "etc/ffshim.h"
#include "fatfs/ff.h"

#include "etc/stub.h"
#define STUB_ADDR  0x80001000
#define STUB_STACK 0x80003000

int screenheight;
int vmode_60hz = 0;
u32 retraceCount;

#include "gfx/kunai_logo.h"
#include "gfx/gfx.h"
#include "spiflash/spiflash.h"
#include "kunaigc/kunaigc.h"
#define KUNAI_VERSION "1.0"

u8 *dol = NULL;
char *path = "KUNAIGC/recovery.dol";

extern lfs_t lfs;
extern lfs_file_t lfs_file;
extern struct lfs_config cfg;

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

    size_t size = f_size(&file);
    dol_alloc(size);
    if (!dol)
    {
        res = 0;
        goto unmount;
    }
    UINT _;
    f_read(&file, dol, size, &_);
    f_close(&file);

unmount:
    kprintf("Unmounting %s\n", slot_name);
    iface->shutdown();
    iface = NULL;

end:
    return res;
}

unsigned int convert_int(unsigned int in)
{
    unsigned int out;
    char *p_in = (char *) &in;
    char *p_out = (char *) &out;
    p_out[0] = p_in[3];
    p_out[1] = p_in[2];
    p_out[2] = p_in[1];
    p_out[3] = p_in[0];
    return out;
}

#define PC_READY 0x80
#define PC_OK    0x81
#define GC_READY 0x88
#define GC_OK    0x89

int load_usb(char slot)
{
    kprintf("Trying USB Gecko in slot %c\n", slot);

    int channel, res = 1;

    switch (slot)
    {
    case 'B':
        channel = 1;
        break;

    case 'A':
    default:
        channel = 0;
        break;
    }

    if (!usb_isgeckoalive(channel))
    {
        kprintf("Not present\n");
        res = 0;
        goto end;
    }

    usb_flush(channel);

    char data;

    kprintf("Sending ready\n");
    data = GC_READY;
    usb_sendbuffer_safe(channel, &data, 1);

    kprintf("Waiting for ack...\n");
    while ((data != PC_READY) && (data != PC_OK))
        usb_recvbuffer_safe(channel, &data, 1);

    if(data == PC_READY)
    {
        kprintf("Respond with OK\n");
        // Sometimes the PC can fail to receive the byte, this helps
        usleep(100000);
        data = GC_OK;
        usb_sendbuffer_safe(channel, &data, 1);
    }

    kprintf("Getting DOL size\n");
    int size;
    usb_recvbuffer_safe(channel, &size, 4);
    size = convert_int(size);

    dol_alloc(size);
    unsigned char* pointer = dol;

    if(!dol)
    {
        res = 0;
        goto end;
    }

    kprintf("Receiving file...\n");
    while (size > 0xF7D8)
    {
        usb_recvbuffer_safe(channel, (void *) pointer, 0xF7D8);
        size -= 0xF7D8;
        pointer += 0xF7D8;
    }
    if(size)
        usb_recvbuffer_safe(channel, (void *) pointer, size);

end:
    return res;
}

int load_lfs(const char * filePath)
{
    int res = 1;

    kprintf("Trying lfs\n");

    //update block_count regarding to flash chip
    u32 jedec_id = kunai_get_jedecID();
    cfg.block_count = (((1 << (jedec_id & 0xFFUL)) - KUNAI_OFFS) / cfg.block_size);

	int err = lfs_mount(&lfs, &cfg);

    if (err != LFS_ERR_OK)
    {
        kprintf("Couldn't mount lfs\n");
        res = 0;
        goto end;
    }

    kprintf("lfs mounted\n");

    kprintf("Reading %s\n", filePath);
    if (lfs_file_open(&lfs, &lfs_file, filePath, LFS_O_RDWR) != LFS_ERR_OK)
    {
        kprintf("Failed to open file\n");
        res = 0;
        goto unmount;
    }

    size_t size = lfs_file_size(&lfs, &lfs_file);
    dol_alloc(size);
    if (!dol)
    {
        res = 0;
        goto unmount;
    }
    lfs_file_read(&lfs, &lfs_file, dol, size);
    lfs_file_close(&lfs, &lfs_file);
unmount:
    kprintf("Unmounting lfs\n");
    lfs_unmount(&lfs);
end:
    return res;
}

GXRModeObj *rmode = NULL;

int main()
{
	VIDEO_Init ();		/*** ALWAYS CALL FIRST IN ANY LIBOGC PROJECT!
					     Not only does it initialise the video
					     subsystem, but also sets up the ogc os
	 ***/

	PAD_Init ();			/*** Initialise pads for input ***/

	// get default video mode
	rmode = VIDEO_GetPreferredMode(NULL);

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


		vmode_60hz = 0;
		break;

	case VI_NTSC:
		// 480 lines (NTSC 60hz)
		vmode_60hz = 1;
		break;

	default:
		// 480 lines (PAL 60Hz)
		vmode_60hz = 1;
		break;
	}

	/* we have component cables, but the preferred mode is interlaced
	 * why don't we switch into progressive?
	 * (user may not have progressive compatible display but component input)*/
	if(VIDEO_HaveComponentCable()) rmode = &TVNtsc480Prog;
	// configure VI
	VIDEO_Configure (rmode);

	/*** Clear framebuffer to black ***/
	VIDEO_ClearFrameBuffer (rmode, __xfb, COLOR_BLACK);

	/*** Set the framebuffer to be displayed at next VBlank ***/
	VIDEO_SetNextFramebuffer (__xfb);

	/*** Get the PAD status updated by libogc ***/
	//		VIDEO_SetPostRetraceCallback (updatePAD);
	VIDEO_SetBlack (0);

	/*** Update the video for next vblank ***/
	VIDEO_Flush ();

	VIDEO_WaitVSync ();		/*** Wait for VBL ***/
	if (rmode->viTVMode & VI_NON_INTERLACE)
		VIDEO_WaitVSync ();

	CON_Init(__xfb, 0, 0, rmode->fbWidth, rmode->xfbHeight, rmode->fbWidth * VI_DISPLAY_PIX_SZ);

	kprintf("\n\nKunaiRecovery - based on iplboot\n");

	// Disable Qoob/KunaiGC
	u32 val = 6 << 24;
	u32 addr = 0xC0000000;
	EXI_Lock(EXI_CHANNEL_0, EXI_DEVICE_1, NULL);
	EXI_Select(EXI_CHANNEL_0, EXI_DEVICE_1, EXI_SPEED8MHZ);
	EXI_Imm(EXI_CHANNEL_0, &addr, 4, EXI_WRITE, NULL);
	EXI_Sync(EXI_CHANNEL_0);
	EXI_Imm(EXI_CHANNEL_0, &val, 4, EXI_WRITE, NULL);
	EXI_Sync(EXI_CHANNEL_0);
	EXI_Deselect(EXI_CHANNEL_0);
	EXI_Unlock(EXI_CHANNEL_0);

	// Set the timebase properly for games
	// Note: fuck libogc and dkppc
	u32 t = ticks_to_secs(SYS_Time());
	settime(secs_to_ticks(t));

	PAD_ScanPads();

	u16 all_buttons_held = (
			PAD_ButtonsHeld(PAD_CHAN0) |
			PAD_ButtonsHeld(PAD_CHAN1) |
			PAD_ButtonsHeld(PAD_CHAN2) |
			PAD_ButtonsHeld(PAD_CHAN3)
	);


	if (all_buttons_held & PAD_TRIGGER_Z) {

		if (load_usb('B')) goto load;

		if (load_fat("sdb", &__io_gcsdb)) goto load;

		if (load_usb('A')) goto load;

		if (load_fat("sda", &__io_gcsda)) goto load;

		if (load_fat("sd2", &__io_gcsd2)) goto load;
	}

	if (load_lfs("KunaiLoader.dol")) goto load;

	load:
	// Wait to exit while the d-pad down direction is held.
	while ((all_buttons_held & PAD_BUTTON_DOWN))
	{
		VIDEO_WaitVSync();
		PAD_ScanPads();
		all_buttons_held = (
				PAD_ButtonsHeld(PAD_CHAN0) |
				PAD_ButtonsHeld(PAD_CHAN1) |
				PAD_ButtonsHeld(PAD_CHAN2) |
				PAD_ButtonsHeld(PAD_CHAN3)
		);
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
	// If we reach here, all attempts to load a DOL failed
	// Since we've disabled the Qoob, we wil reboot to the Nintendo IPL
	return 0;
}
