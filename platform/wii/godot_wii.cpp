#include "main/main.h"
#include "os_wii.h"

#include <gccore.h>
#include <wiiuse/wpad.h>
#include <fat.h>
#include <dirent.h>
#include <stdio.h>

static void *xfb = NULL;
static GXRModeObj *rmode = NULL;

extern "C"
{
    int main(int argc, char **argv) {
        // This function initialises the attached controllers
        WPAD_Init();
        
        OS_Wii os;

        Error err;
        if(argc > 1)
            err = Main::setup(argv[0], argc-1, argv+1);
        else if(argc == 1)
            err = Main::setup(argv[0], 0, NULL);
        else
            err = Main::setup("/godot_app.elf", 0, NULL);

        if(err != OK)
            exit(255);
        
        if (Main::start())
            os.run();
        Main::cleanup();
        
        exit(0);
    }
}