#include "esp_log.h"
#include "freertos/idf_additions.h"
#include "freertos/projdefs.h"
#define TAG "Tune Bit"

int app_main() {
    vTaskDelay(pdMS_TO_TICKS(1000));
    ESP_LOGI(TAG, "Starting");
    
    return 1;
}
