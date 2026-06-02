#include "onx3248g035_bsp.h"

#include "esp_log.h"
#include "nvs_flash.h"

static const char *TAG = "onx_smoke";

void app_main(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "Starting ONX3248G035 BSP smoke firmware");
    ESP_ERROR_CHECK(onx_bsp_smoke_run());
    ESP_LOGI(TAG, "ONX3248G035 BSP smoke firmware finished");
}
