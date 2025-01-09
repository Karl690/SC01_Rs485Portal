#include "L_Core/devices/conf_WT32SCO1-Plus.h"
#include "L_Core/ui/ui.h"
/*
MIT License

Copyright (c) 2022 Sukesh Ashok Kumar
Copyright (c) 2023 Janick Bergeron  <janick@bergeron.com>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/


#include "lv_conf.h"
#include <lvgl.h>
#include <freertos/FreeRTOS.h>

#define LV_TICK_PERIOD_MS 1
    
/*********************
 *  THEME DEFINES
 *********************/
#define MODE_DARK 1
#define RADIUS_DEFAULT (disp_size == DISP_LARGE ? lv_disp_dpx(theme.disp, 12) : lv_disp_dpx(theme.disp, 8))

/*SCREEN*/
#define LIGHT_COLOR_SCR        lv_palette_lighten(LV_PALETTE_GREY, 4)
#define LIGHT_COLOR_CARD       lv_color_white()
#define LIGHT_COLOR_TEXT       lv_palette_darken(LV_PALETTE_GREY, 4)
#define LIGHT_COLOR_GREY       lv_palette_lighten(LV_PALETTE_GREY, 2)
#define DARK_COLOR_SCR         lv_color_hex(0x15171A)
#define DARK_COLOR_CARD        lv_color_hex(0x282b30)
#define DARK_COLOR_TEXT        lv_palette_lighten(LV_PALETTE_GREY, 5)
#define DARK_COLOR_GREY        lv_color_hex(0x2f3237)

#define TRANSITION_TIME         LV_THEME_DEFAULT_TRANSITION_TIME
#define BORDER_WIDTH            lv_disp_dpx(theme.disp, 2)
#define OUTLINE_WIDTH           lv_disp_dpx(theme.disp, 3)

#define PAD_DEF     (disp_size == DISP_LARGE ? lv_disp_dpx(theme.disp, 24) : disp_size == DISP_MEDIUM ? lv_disp_dpx(theme.disp, 20) : lv_disp_dpx(theme.disp, 16))
#define PAD_SMALL   (disp_size == DISP_LARGE ? lv_disp_dpx(theme.disp, 14) : disp_size == DISP_MEDIUM ? lv_disp_dpx(theme.disp, 12) : lv_disp_dpx(theme.disp, 10))
#define PAD_TINY   (disp_size == DISP_LARGE ? lv_disp_dpx(theme.disp, 8) : disp_size == DISP_MEDIUM ? lv_disp_dpx(theme.disp, 6) : lv_disp_dpx(theme.disp, 2))

#define BUFF_SIZE 20
// #define LVGL_DOUBLE_BUFFER



extern bool force_touch;
extern bool force_touched;
extern uint16_t force_touchx, force_touchy;

extern uint8_t* ui_plot_buffer;
extern bool display_screenshot;
extern bool display_screenshot_completed;

extern uint8_t* dispaly_snapshot_buffer565;
extern uint8_t* display_snapshot_compress_buffer;
extern size_t		display_compress_buffer_size;
extern lv_img_dsc_t display_caputure_img_dsc;
esp_err_t InitLCDAndLVGL();

// Display callback to flush the buffer to screen
void display_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p);

/* Setting up tick task for lvgl */
void lv_tick_task(void *arg);

void gui_task(void *args);

// Touchpad callback to read the touchpad
void touchpad_read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data);

bool display_dump_buffer();
void display_reset_capture_buffer();


//////////////////// bmp function //////////////////////////////////
#pragma pack(push, 1) // Ensure structure packing matches BMP format
typedef struct {
	uint16_t bfType; // Bitmap file type (must be 'BM')
	uint32_t bfSize; // Size of the file in bytes
	uint16_t bfReserved1; // Reserved (must be 0)
	uint16_t bfReserved2; // Reserved (must be 0)
	uint32_t bfOffBits; // Offset to pixel data
} BMPFileHeader;

typedef struct {
	uint32_t biSize; // Size of the DIB header
	int32_t biWidth; // Image width
	int32_t biHeight; // Image height (negative for top-down)
	uint16_t biPlanes; // Number of color planes (must be 1)
	uint16_t biBitCount; // Bits per pixel (24 for RGB)
	uint32_t biCompression; // Compression type (0 for uncompressed)
	uint32_t biSizeImage; // Image size (can be 0 for uncompressed)
	int32_t biXPelsPerMeter; // Horizontal resolution (pixels per meter)
	int32_t biYPelsPerMeter; // Vertical resolution (pixels per meter)
	uint32_t biClrUsed; // Number of colors in the color palette
	uint32_t biClrImportant; // Important colors (0 means all are important)
} BMPInfoHeader;
#pragma pack(pop)

bool save_bmp(const char *filename, uint16_t* buffer, int width, int height);