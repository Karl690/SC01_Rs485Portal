#include "display.h"
#include  "configure.h"
#include <lvgl.h>
#include "../server/server.h"
#include "K_Core/serial/serial.h"
#include "K_Core/communication/communication.h"
/*** Setup screen resolution for LVGL ***/
static const uint16_t screenWidth =  SCREEN_WIDTH;// TFT_WIDTH;
static const uint16_t screenHeight = SCREEN_HEIGHT;// TFT_HEIGHT;

static lv_disp_draw_buf_t draw_buf;

static lv_disp_t *disp;
uint8_t* ui_plot_buffer;
uint8_t* display_snapshot_compress_buffer;
uint8_t* dispaly_snapshot_buffer565;
size_t		display_compress_buffer_size;
	
static LGFX lcd; // declare display variable

static TaskHandle_t g_lvgl_task_handle;

bool force_touch = 0;
bool force_touched = 0;
uint16_t force_touchx, force_touchy;

bool display_screenshot = false;
bool display_screenshot_completed = false;

lv_img_dsc_t display_caputure_img_dsc;
esp_err_t InitLCDAndLVGL()
{
	// Setting display to landscape
	
	lcd.init();
	lcd.initDMA();
#ifdef PORTRAIT
	
#else
	//if (lcd.width() < lcd.height()) lcd.setRotation(lcd.getRotation() ^ 2);
	lcd.setRotation(3);
#endif
	lcd.setColorDepth(16);	
	lcd.setBrightness(128);
	//lcd.fillScreen(TFT_BLACK);
	lv_init();
	
	/* LVGL : Setting up buffer to use for display */
#if defined(LVGL_DOUBLE_BUFFER)
	//    EXT_RAM_BSS_ATTR lv_color_t * buf1 = (lv_color_t *)malloc(screenWidth * BUFF_SIZE * sizeof(lv_color_t));
	//     EXT_RAM_BSS_ATTR lv_color_t * buf2 = (lv_color_t *)malloc(screenWidth * BUFF_SIZE * sizeof(lv_color_t));

	lv_color_t * buf1 = (lv_color_t *)heap_caps_malloc(screenWidth * BUFF_SIZE * sizeof(lv_color_t), MALLOC_CAP_DMA);
	lv_color_t * buf2 = (lv_color_t *)heap_caps_malloc(screenWidth * BUFF_SIZE * sizeof(lv_color_t), MALLOC_CAP_DMA);

	lv_disp_draw_buf_init(&draw_buf, buf1, buf2, screenWidth * BUFF_SIZE);    
#else
	// EXT_RAM_BSS_ATTR 
	//lv_color_t * buf1 = (lv_color_t *)lv_mem_alloc(screenWidth * BUFF_SIZE * sizeof(lv_color_t));
	lv_color_t * buf1 = (lv_color_t *)heap_caps_malloc(screenWidth * BUFF_SIZE * sizeof(lv_color_t), MALLOC_CAP_DMA);
	lv_disp_draw_buf_init(&draw_buf, buf1, NULL, screenWidth * BUFF_SIZE);
#endif
	/*** LVGL : Setup & Initialize the display device driver ***/
	static lv_disp_drv_t disp_drv;
	lv_disp_drv_init(&disp_drv);
	disp_drv.hor_res = screenWidth;
	disp_drv.ver_res = screenHeight;
	disp_drv.flush_cb = display_flush;
	disp_drv.draw_buf = &draw_buf;
	disp_drv.sw_rotate = 1;
	disp = lv_disp_drv_register(&disp_drv);

	//*** LVGL : Setup & Initialize the input device driver ***
	static lv_indev_drv_t indev_drv;
	lv_indev_drv_init(&indev_drv);
	indev_drv.type = LV_INDEV_TYPE_POINTER;
	indev_drv.read_cb = touchpad_read;
	lv_indev_drv_register(&indev_drv);

	/* Create and start a periodic timer interrupt to call lv_tick_inc */
	const esp_timer_create_args_t lv_periodic_timer_args = {
		.callback = &lv_tick_task,
		.arg = NULL,
		.dispatch_method = ESP_TIMER_TASK,
		.name = "periodic_gui",
		.skip_unhandled_events = true
	};
	esp_timer_handle_t lv_periodic_timer;
	ESP_ERROR_CHECK(esp_timer_create(&lv_periodic_timer_args, &lv_periodic_timer));
	ESP_ERROR_CHECK(esp_timer_start_periodic(lv_periodic_timer, LV_TICK_PERIOD_MS * 1000));

	int err = xTaskCreatePinnedToCore(gui_task, "lv gui", 1024 * 15, NULL, 10, &g_lvgl_task_handle, 0);
	if (!err)
	{
		//ESP_LOGE(TAG, "Create task for LVGL failed");
		if (lv_periodic_timer) esp_timer_delete(lv_periodic_timer);
		return ESP_FAIL;
	}
	
	//allocate the buf for snapshot.
	ui_plot_buffer = (uint8_t*)heap_caps_malloc((SCREEN_WIDTH-0) * (SCREEN_HEIGHT-0)* sizeof(lv_color_t), MALLOC_CAP_SPIRAM);	
	dispaly_snapshot_buffer565 = (uint8_t*)heap_caps_malloc(SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(lv_color_t), MALLOC_CAP_SPIRAM);
	display_snapshot_compress_buffer = (uint8_t*)heap_caps_malloc(SCREEN_WIDTH * SCREEN_HEIGHT* sizeof(lv_color_t), MALLOC_CAP_SPIRAM);
	display_caputure_img_dsc.data = dispaly_snapshot_buffer565;
	display_caputure_img_dsc.header.w = SCREEN_WIDTH;
	display_caputure_img_dsc.header.h = SCREEN_HEIGHT;
	display_caputure_img_dsc.header.cf = LV_IMG_CF_TRUE_COLOR;
	return ESP_OK;
}

void display_reset_capture_buffer()
{
	memset(dispaly_snapshot_buffer565, 0, sizeof(SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(lv_color_t)));
	memset(display_snapshot_compress_buffer, 0, sizeof(SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(lv_color_t)));
}

// Display callback to flush the buffer to screen
void display_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p)
{
	uint32_t w = (area->x2 - area->x1 + 1);
	uint32_t h = (area->y2 - area->y1 + 1);
	lcd.startWrite();
	lcd.setAddrWindow(area->x1, area->y1, w, h);
	lcd.pushImageDMA(area->x1, area->y1, area->x2 - area->x1 + 1, area->y2 - area->y1 + 1, (lgfx::swap565_t *)&color_p->full);
	lcd.endWrite();
	
	lv_disp_flush_ready(disp);
}

size_t compresss_rgb565(const uint16_t* rgb_buf, uint8_t* compress_buffer, uint32_t width, uint32_t height) {
	uint16_t old_pixel = 0xffff;
	uint16_t counter = 0;
	uint32_t pos = 0;
	
	for (uint32_t i = 0; i < width * height; i++) {
		uint16_t pixel = rgb_buf[i];
		if (old_pixel == pixel)
		{
			counter++;
			if (counter == 0xffff)
			{
				memcpy(&compress_buffer[pos], &counter, 2);
				pos += 2;
				memcpy(&compress_buffer[pos], &pixel, 2);
				pos += 2;
				counter = 1;
				old_pixel = pixel;
			}
		}
		else
		{
			if (i > 0)
			{
				memcpy(&compress_buffer[pos], &counter, 2);
				pos += 2;
			}
			memcpy(&compress_buffer[pos], &pixel, 2);
			pos += 2;
			counter = 1;
			old_pixel = pixel;
		}
	}
	memcpy(&compress_buffer[pos], &counter, 2); pos += 2;
	pos += 2;
	return pos;
}
bool display_dump_buffer()
{
	lv_area_t area;
	lv_obj_t* screen = lv_scr_act();
	lv_obj_get_coords(screen, &area);
	uint32_t size = SCREEN_WIDTH*SCREEN_HEIGHT * sizeof(lv_color_t);
	lv_snapshot_take_to_buf(screen, LV_IMG_CF_TRUE_COLOR, &display_caputure_img_dsc, dispaly_snapshot_buffer565, size);
	
	display_compress_buffer_size = compresss_rgb565((uint16_t*)dispaly_snapshot_buffer565, display_snapshot_compress_buffer, SCREEN_WIDTH, SCREEN_HEIGHT);
	
	return display_compress_buffer_size > 0;
}
/* Setting up tick task for lvgl */
void lv_tick_task(void *arg)
{
	(void)arg;
	lv_tick_inc(LV_TICK_PERIOD_MS);
}

void gui_task(void *args)
{
	//ESP_LOGI(TAG, "Start to run LVGL");
	while (1) {
		vTaskDelay(pdMS_TO_TICKS(10));

		/* Try to take the semaphore, call lvgl related function on success */
	   // lvglLock lock;
		//if (OpcHeartBeatLabel) lv_label_set_text_fmt(OpcHeartBeatLabel, "#ff00ff %d #", (int)OpcHeartBeat);
		lv_task_handler();
		//lv_timer_handler_run_in_period(5); /* run lv_timer_handler() every 5ms */
	}
}


// Touchpad callback to read the touchpad
void touchpad_read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data)
{
	uint16_t touchX = 0, touchY = 0;
	bool touched = 0;
	if (force_touch) 
	{
		// Manually issue a touch event
		touched = force_touched;
		touchX = force_touchx;
		touchY = force_touchy;
		
	}
	else
	{
		touched = lcd.getTouch(&touchX, &touchY);
	}
	
	
	if (!touched)
	{
		data->state = LV_INDEV_STATE_REL;
	}
	else
	{
		data->state = LV_INDEV_STATE_PR;

		// Set the coordinates
		data->point.x = touchX;
		data->point.y = touchY;
	}
	force_touch = false;
}

void rgb565_to_rgb888(uint16_t color, uint8_t *r, uint8_t *g, uint8_t *b) {
	// ----------- Method 1 --------------
	uint16_t rgb565 = ((color & 0xff) << 8) + ((color & 0xff00) >> 8); //invert byte
	uint8_t red = (rgb565 >> 11) & 0x1F; // 5 bits
	uint8_t green = (rgb565 >> 5) & 0x3F; // 6 bits
	uint8_t blue = rgb565 & 0x1F; // 5 bits

	// Scale to 8 bits
	*r = (red * 255) / 31; // Scale red
	*g = (green * 255) / 63; // Scale green
	*b = (blue * 255) / 31; // Scale blue
//	
	// ----------- Method 2 --------------
//	*r = (rgb565 >> 11) & 0x1F; // Extract red
//	*r = (*r << 3) | (*r >> 2); // Scale red to 8 bits
//
//	*g = (rgb565 >> 5) & 0x3F; // Extract green
//	*g = (*g << 2) | (*g >> 4); // Scale green to 8 bits
//
//	*b = rgb565 & 0x1F; // Extract blue
//	*b = (*b << 3) | (*b >> 2); // Scale blue to 8 bits
}
bool save_bmp(const char *filename, uint16_t* buffer, int width, int height) {
	FILE *file = fopen(filename, "wb");
	if (!file) {
		return false;
	}

	// BMP headers
	BMPFileHeader fileHeader;
	BMPInfoHeader infoHeader;

	int rowSize = (3 * width + 3) & ~3; // Each row is padded to a multiple of 4 bytes
	int dataSize = rowSize * height;

	// Populate the BMP file header
	fileHeader.bfType = 0x4D42; // 'BM'
	fileHeader.bfSize = sizeof(BMPFileHeader) + sizeof(BMPInfoHeader) + dataSize;
	fileHeader.bfReserved1 = 0;
	fileHeader.bfReserved2 = 0;
	fileHeader.bfOffBits = sizeof(BMPFileHeader) + sizeof(BMPInfoHeader);

	// Populate the BMP info header
	infoHeader.biSize = sizeof(BMPInfoHeader);
	infoHeader.biWidth = width;
	infoHeader.biHeight = -height; // Negative for top-down
	infoHeader.biPlanes = 1;
	infoHeader.biBitCount = 24; // 24-bit RGB
	infoHeader.biCompression = 0; // No compression
	infoHeader.biSizeImage = dataSize;
	infoHeader.biXPelsPerMeter = 2835; // 72 DPI
	infoHeader.biYPelsPerMeter = 2835; // 72 DPI
	infoHeader.biClrUsed = 0;
	infoHeader.biClrImportant = 0;

	// Write headers to file
	fwrite(&fileHeader, sizeof(BMPFileHeader), 1, file);
	fwrite(&infoHeader, sizeof(BMPInfoHeader), 1, file);
	uint8_t r, g, b;
	// Write pixel data (convert LVGL buffer to BMP format)
	uint8_t *row = (uint8_t*)malloc(rowSize);
	for (int y = 0; y < height; y++) {
		memset(row, 0, rowSize);
		for (int x = 0; x < width; x++) {
			uint16_t color = buffer[y * width + x];
			int index = x * 3;
			rgb565_to_rgb888(color, &r, &g, &b);
			row[index + 2] = r;
			row[index + 1] = g;
			row[index + 0] = b;
		}
		fwrite(row, rowSize, 1, file);
	}

	// Clean up
	free(row);
	// fflush(file);
	fclose(file);

	return true;
}