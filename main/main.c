#include "spi_lcd.h"
#include "gamepad.h"
#include "cboy.h"

#include "freertos/FreeRTOS.h"
#include "esp_event.h"
#include "esp_event_loop.h"
#include "nvs_flash.h"
#include "esp_partition.h"

extern int cmd_info_impl();

unsigned char *getromdata() {
	unsigned char* romdata;
	const esp_partition_t* part;
	spi_flash_mmap_handle_t hrom;
	esp_err_t err;
	nvs_flash_init();
	part=esp_partition_find_first(0x40, 1, NULL);
	if (part==0) printf("Couldn't find rom part!\n");
	err=esp_partition_mmap(part, 0, 3*1024*1024, SPI_FLASH_MMAP_DATA, (const void**)&romdata, &hrom);
	if (err!=ESP_OK) printf("Couldn't map rom part!\n");
	printf("Initialized. ROM@%p\n", romdata);
    return (unsigned char*)romdata;
}

void app_main(void)
{
	// Test rom is read properly
	// cmd_info_impl();

	// Or run the rom.
	cboy_run();

	printf("Exitting cleanly from main\n");
}
