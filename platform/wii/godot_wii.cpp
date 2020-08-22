#include "main/main.h"
#include "os_wii.h"

#include <gccore.h>
#include <wiiuse/wpad.h>
#include <fat.h>
#include <dirent.h>
#include <stdio.h>

static void *xfb = NULL;
static GXRModeObj *rmode = NULL;

#define INIT_CONSOLE

extern "C"
{
    int main(int argc, char **argv) {
        #ifdef INIT_CONSOLE
        // Initialise the video system
        VIDEO_Init();

        // This function initialises the attached controllers
        WPAD_Init();

        // Obtain the preferred video mode from the system
        // This will correspond to the settings in the Wii menu
        rmode = VIDEO_GetPreferredMode(NULL);

        // Allocate memory for the display in the uncached region
        xfb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));

        // Initialise the console, required for printf
        console_init(xfb,20,20,rmode->fbWidth,rmode->xfbHeight,rmode->fbWidth*VI_DISPLAY_PIX_SZ);

        // Set up the video registers with the chosen mode
        VIDEO_Configure(rmode);

        // Tell the video hardware where our display memory is
        VIDEO_SetNextFramebuffer(xfb);

        // Make the display visible
        VIDEO_SetBlack(FALSE);

        // Flush the video register changes to the hardware
        VIDEO_Flush();

        // Wait for Video setup to complete
        VIDEO_WaitVSync();
        if(rmode->viTVMode&VI_NON_INTERLACE) VIDEO_WaitVSync();

        // The console understands VT terminal escape codes
        // This positions the cursor on row 2, column 0
        // we can use variables for this with format codes too
        // e.g. printf ("\x1b[%d;%dH", row, column );
        printf("\x1b[2;0H");

        printf("Hello World!\n");

        VIDEO_WaitVSync();

        #endif

        OS_Wii os;

        Error err;
        err = Main::setup("godot_app", argc, argv); // INCLUDING THIS LINE CRASHES

        if(err != OK)
            exit(255);
        
        if (Main::start())
            os.run();
        Main::cleanup();
    }
}