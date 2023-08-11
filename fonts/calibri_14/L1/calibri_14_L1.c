/*
This file is automatically generated
calibri_14_L1
C Source
*/

#include "App_Common.h"

#define FONT_HANDLE       (1)
#define FONT_FILE_ADDRESS (RAM_G + 153600)
#define FIRST_CHARACTER   (32)

void Load_Font()
{
	Gpu_CoCmd_Dlstart(phost);
	App_WrCoCmd_Buffer(phost, CLEAR(1, 1, 1));
	App_WrCoCmd_Buffer(phost, COLOR_RGB(255, 255, 255));
	Gpu_Hal_LoadImageToMemory(phost, "path\\to\\calibri_14_L1.raw", FONT_FILE_ADDRESS, LOAD);

	Gpu_CoCmd_SetFont2(phost, FONT_HANDLE, FONT_FILE_ADDRESS, FIRST_CHARACTER);
	Gpu_CoCmd_Text(phost, 0, 0, FONT_HANDLE, 0, "AaBbCcDdEeFf");
	App_WrCoCmd_Buffer(phost, DISPLAY());
	Gpu_CoCmd_Swap(phost);
	App_Flush_Co_Buffer(phost);
	Gpu_Hal_WaitCmdfifo_empty(phost);
}

/* end of file */
