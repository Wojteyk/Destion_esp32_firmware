#include "sht40.h"
#include "firebase.h"
#include "uart_connection.h"

#define I2C_MASTER_SCL 19
#define I2C_MASTER_SDA 21
#define I2C_MASTER_NUM 0
#define I2C_MASTER_FREQ_HZ 100000
#define I2C_MASTER_TX_BUF_DISABLE   0      
#define I2C_MASTER_RX_BUF_DISABLE   0       
#define I2C_MASTER_TIMEOUT_MS       100

#define SHT40_ADDR 0x44
#define CMD_MEASURE_HIGH_PRECISION 0xFD

static const char* TAG = "SHT40";

static float humidity = 0;
static float temperature = 0;

static uint8_t calculateCrc(const uint8_t *data)
{
    uint8_t crc = 0xFF;
    for(int i = 0; i < 2; i++)
    {
        crc ^= data[i];
        for(int bit = 0; bit < 8; bit++)
        {
            if(crc & 0x80)
            {
                crc = (crc << 1) ^ 0x31;
            } 
            else 
            {
                crc <<= 1;
            }
        }
    }
    return crc;
}

void sht40_init()
{
    int i2cMasterPort = I2C_MASTER_NUM;

    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA,
        .scl_io_num = I2C_MASTER_SCL,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };

    esp_err_t err = i2c_param_config(i2cMasterPort, &conf);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Wrong config: %s", esp_err_to_name(err));
        return;
    }

    err = i2c_driver_install(i2cMasterPort, conf.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0 );
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Unable to install: %s", esp_err_to_name(err));
        return;
    }
}

uint8_t sht40_read()
{
    uint8_t cmd = CMD_MEASURE_HIGH_PRECISION;
    uint8_t data[6];

    esp_err_t err = i2c_master_write_to_device(I2C_MASTER_NUM, SHT40_ADDR, &cmd, 1, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Unable to send data: ", esp_err_to_name(err));
        return 0;
    }

    vTaskDelay(pdMS_TO_TICKS(10));

    err = i2c_master_read_from_device(I2C_MASTER_NUM, SHT40_ADDR, data, 6, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
    if (err == ESP_OK)
    {

        uint16_t temp_uncut = (data[0]<<8) | data[1];
        uint16_t humi_uncut = (data[3]<<8) | data[4];

        if(calculateCrc(data) != data[2]){
            ESP_LOGE(TAG, "Checksum problem: ");
            return 0; 
        }

        if(calculateCrc(&data[3]) != data[5]){
            ESP_LOGE(TAG, "Checksum problem: ");
            return 0; 
        }

        humidity = -6.0f + 125.0f * humi_uncut/65535.0f;

        if(humidity > 100) humidity = 100;
        else if(humidity < 0) humidity = 0;

        temperature  = -45.0f + (175.0f * temp_uncut/65535.0f);
        ESP_LOGI(TAG, "Data: %0.2f", temperature);
        return 1;
    }
    else 
    {
        ESP_LOGE(TAG, "Unable to read data: %s", esp_err_to_name(err));
        return 0;
    }
}

void sht40_uart_task(void *pvParameter)
{
    while(1)
    {
        uint8_t status = sht40_read();
        if(status)
        {
            uart_sendSensorsData(temperature, humidity);
        }
        else
        {
            ESP_LOGE(TAG, "Unable to send data: %d", status);
        }
        vTaskDelay(pdMS_TO_TICKS(300000)); // 5 minutes
    }
}

void sht40_firebase_task(void* pvParameters)
{
    while(1)
    {
        uint8_t status = sht40_read();
        if(status)
        {
            firebase_put("DHT11/temperature", temperature);
            firebase_put("DHT11/humidity", humidity);
        }
        else
        {
            ESP_LOGE(TAG, "Unable to send data: %d", status);
        }
        vTaskDelay(pdMS_TO_TICKS(300000)); // 5 minutes
    }
}