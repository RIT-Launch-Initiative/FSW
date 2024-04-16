// Code for testing sensors but stuff that was getting in the way in main
#include "testing.h"

#define NUM_SAMPLES 5
float samples[NUM_SAMPLES] = {0.0f};
int sample_index = 0;
float sample_sum = 0;

int samplesi[NUM_SAMPLES] = {0.0f};
int samplei_index = 0;
float samplei_sum = 0;

// add a new sample, return the moving avg
float add_sample(float sample) {
  float old = samples[sample_index];
  samples[sample_index] = sample;
  sample_index = (sample_index + 1) % NUM_SAMPLES;
  sample_sum -= old;
  sample_sum += sample;
  return sample_sum / (float)NUM_SAMPLES;
}

float add_sample_int(int samplei) {
  float old = samplesi[samplei_index];
  samplesi[samplei_index] = samplei;
  samplei_index = (samplei_index + 1) % NUM_SAMPLES;
  samplei_sum -= old;
  samplei_sum += samplei;
  return samplei_sum / (float)NUM_SAMPLES;
}

void adc_printout(const struct adc_dt_spec *channel) {
  int32_t buf;
  struct adc_sequence sequence = {
      .buffer = &buf,
      /* buffer size in bytes, not number of samples */
      .buffer_size = sizeof(buf),
  };
  int frame = 0;
  while (true) {

    int err = adc_sequence_init_dt(channel, &sequence);
    if (err < 0) {
      printk("Could not init seq: %d", err);
    }
    err = adc_read_dt(channel, &sequence);
    if (err < 0) {
      printk("Could not read (%d)\n", err);
      continue;
    }
    int32_t val = buf;
    float volts = 2.4f * ((float)val) / ((float)0x7fffff);

    float avg = add_sample(volts);

    printk("%06d,%06x,%d,%2.8f,%2.8f\n", frame, val, val, (double)volts,
           (double)avg);
    frame++;

    k_msleep(100);
  }
}