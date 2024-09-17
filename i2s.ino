#include "driver/i2s.h"
#include <math.h>

#define I2S_MIC_CHANNEL I2S_NUM_0
#define I2S_SPEAKER_CHANNEL I2S_NUM_1
#define SAMPLE_RATE 44100

// INMP441 Pin Definitions
#define I2S_MIC_WS 32
#define I2S_MIC_SCK 33
#define I2S_MIC_SD 25

// MAX98375A Pin Definitions
#define I2S_SPEAKER_WS 2    // LRC
#define I2S_SPEAKER_BCLK 15 // BCLK
#define I2S_SPEAKER_DIN 13  // DIN
#define I2S_SPEAKER_SD 12   // SD (Shutdown pin control)

void setup()
{
  Serial.begin(115200);

  // I2S configuration for microphone (input)
  i2s_config_t i2s_config_in = {
      .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
      .sample_rate = SAMPLE_RATE,
      .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT, // Microphone sends 32-bit data
      .channel_format = I2S_CHANNEL_FMT_ONLY_RIGHT, // Mono input from the mic
      .communication_format = I2S_COMM_FORMAT_I2S_MSB,
      .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
      .dma_buf_count = 8,
      .dma_buf_len = 64};

  i2s_pin_config_t pin_config_in = {
      .bck_io_num = I2S_MIC_SCK,
      .ws_io_num = I2S_MIC_WS,
      .data_out_num = I2S_PIN_NO_CHANGE,
      .data_in_num = I2S_MIC_SD};

  i2s_driver_install(I2S_MIC_CHANNEL, &i2s_config_in, 0, NULL);
  i2s_set_pin(I2S_MIC_CHANNEL, &pin_config_in);

  // I2S configuration for speaker (output)
  i2s_config_t i2s_config_out = {
      .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
      .sample_rate = SAMPLE_RATE,
      .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT, // 16-bit output for the speaker
      .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT, // Stereo output
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

  // Enable the amplifier by setting the SD pin HIGH
  pinMode(I2S_SPEAKER_SD, OUTPUT);
  digitalWrite(I2S_SPEAKER_SD, HIGH); // Enable amplifier
}

void loop()
{
  size_t bytesRead, bytesWritten;
  int32_t micBuffer[256];     // Buffer for 32-bit microphone data
  int16_t speakerBuffer[512]; // Buffer for 16-bit stereo speaker data

  // Read data from the microphone
  esp_err_t err = i2s_read(I2S_MIC_CHANNEL, micBuffer, sizeof(micBuffer), &bytesRead, portMAX_DELAY);
  if (err == ESP_OK && bytesRead > 0)
  {
    // Calculate RMS (Root Mean Square) for volume level
    int64_t sumOfSquares = 0;
    int samplesRead = bytesRead / sizeof(int32_t); // Total number of 32-bit samples

    for (int i = 0; i < samplesRead; i++)
    {
      int32_t sample = micBuffer[i];
      sumOfSquares += (int64_t)sample * sample;

      // Convert 32-bit sample to 16-bit and duplicate for stereo output
      speakerBuffer[2 * i] = sample >> 16;     // Left channel
      speakerBuffer[2 * i + 1] = sample >> 16; // Right channel
    }

    // Calculate RMS for the mic input
    float rms = sqrt(sumOfSquares / samplesRead);
    int volumeLevel = min((int)(rms / 1000000), 15); // Scale RMS to 0-15 range

    // Display volume meter in Serial monitor
    Serial.print("Volume: ||");
    for (int i = 0; i < volumeLevel; i++)
    {
      Serial.print("X");
    }
    for (int i = volumeLevel; i < 15; i++)
    {
      Serial.print(" ");
    }
    Serial.println("||");

    // Write microphone data (now in 16-bit stereo format) to the speaker
    i2s_write(I2S_SPEAKER_CHANNEL, speakerBuffer, samplesRead * 2 * sizeof(int16_t), &bytesWritten, portMAX_DELAY);
  }
  else
  {
    // Error handling or no data read from the microphone
    Serial.println("Error reading from microphone or no data.");
  }
}
