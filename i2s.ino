#include "driver/i2s.h"

#define I2S_SPEAKER_CHANNEL I2S_NUM_1
#define SAMPLE_RATE 44100

// MAX98375A Pin Definitions
#define I2S_SPEAKER_WS 2    // LRC
#define I2S_SPEAKER_BCLK 15 // BCLK
#define I2S_SPEAKER_DIN 13  // DIN
#define I2S_SPEAKER_SD 12   // SD (Shutdown pin control)

void setup()
{
  Serial.begin(115200);

  // I2S configuration for MAX98375A speaker (output)
  i2s_config_t i2s_config_out = {
      .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
      .sample_rate = SAMPLE_RATE,
      .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT, // 16-bit data for the speaker
      .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT, // Stereo output (same signal on both channels)
      .communication_format = I2S_COMM_FORMAT_I2S_MSB,
      .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
      .dma_buf_count = 8,
      .dma_buf_len = 64};

  i2s_pin_config_t pin_config_out = {
      .bck_io_num = I2S_SPEAKER_BCLK,
      .ws_io_num = I2S_SPEAKER_WS,
      .data_out_num = I2S_SPEAKER_DIN,
      .data_in_num = I2S_PIN_NO_CHANGE};

  i2s_driver_install(I2S_SPEAKER_CHANNEL, &i2s_config_out, 0, NULL);
  i2s_set_pin(I2S_SPEAKER_CHANNEL, &pin_config_out);

  // Ensure the shutdown pin for MAX98375A is enabled
  pinMode(I2S_SPEAKER_SD, OUTPUT);
  digitalWrite(I2S_SPEAKER_SD, HIGH); // Enable amplifier
}

void loop()
{
  // Generate a continuous square wave test tone
  int16_t testTone[512];
  for (int i = 0; i < 512; i++)
  {
    testTone[i] = (i % 2 == 0) ? 30000 : -30000; // Simple square wave
  }

  size_t bytesWritten;

  // Continuously send the test tone to the speaker
  while (true)
  {
    i2s_write(I2S_SPEAKER_CHANNEL, testTone, sizeof(testTone), &bytesWritten, portMAX_DELAY);

    // Optionally, add a slight delay between writes to avoid overloading the buffer
    delay(100); // Adjust the delay if necessary for a smoother tone
  }
}
