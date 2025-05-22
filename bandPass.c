#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "lv2/core/lv2.h"

#define BANDPASS_URI "http://example.org/plugins/bandPass"

typedef struct {
    float* input;
    float* output;
    float* cutoff_control;
    float* bypass;
    float* quality_control;
    //float* band_gain;
    
    float samplerate;

    uint32_t ds_counter;
    float ds_hold_sample;

    // Estado do filtro (biquad)
    float x1, x2;
    float y1, y2;

    // Coeficientes
    float b0, b1, b2, a1, a2;

    float hp_x1, hp_x2, hp_y1, hp_y2;
    float hp_b0, hp_b1, hp_b2, hp_a1, hp_a2;

    float lp_x1, lp_x2, lp_y1, lp_y2;
    float lp_b0, lp_b1, lp_b2, lp_a1, lp_a2;
    
} Bandpass;

static void update_filter(Bandpass* self) {
    float norm = *(self->cutoff_control);  // 0.0 a 1.0

    float center = 2500.0f;  // Hz
    float min_bandwidth = 100.0f;   // faixa mínima: 100 Hz
    float max_bandwidth = 4800.0f;  // faixa máxima: 4800 Hz

    float bandwidth = min_bandwidth + norm * (max_bandwidth - min_bandwidth);
    float delta = bandwidth / 2.0f;

    float f1 = center - delta;  // high-pass
    float f2 = center + delta;  // low-pass

    if (f1 < 20.0f) f1 = 20.0f;  // evita valores absurdamente baixos
    if (f2 > self->samplerate / 2.0f) f2 = self->samplerate / 2.0f - 100.0f;

    float Q = 0.707f;  // butterworth

    // High-pass
    {
        float w0 = 2.0f * M_PI * f1 / self->samplerate;
        float alpha = sinf(w0) / (2.0f * Q);
        float cosw0 = cosf(w0);

        float b0 =  (1 + cosw0) / 2.0f;
        float b1 = -(1 + cosw0);
        float b2 =  (1 + cosw0) / 2.0f;
        float a0 =   1 + alpha;
        float a1 =  -2 * cosw0;
        float a2 =   1 - alpha;

        self->hp_b0 = b0 / a0;
        self->hp_b1 = b1 / a0;
        self->hp_b2 = b2 / a0;
        self->hp_a1 = a1 / a0;
        self->hp_a2 = a2 / a0;
    }

    // Low-pass
    {
        float w0 = 2.0f * M_PI * f2 / self->samplerate;
        float alpha = sinf(w0) / (2.0f * Q);
        float cosw0 = cosf(w0);

        float b0 =  (1 - cosw0) / 2.0f;
        float b1 =   1 - cosw0;
        float b2 =  (1 - cosw0) / 2.0f;
        float a0 =   1 + alpha;
        float a1 =  -2 * cosw0;
        float a2 =   1 - alpha;

        self->lp_b0 = b0 / a0;
        self->lp_b1 = b1 / a0;
        self->lp_b2 = b2 / a0;
        self->lp_a1 = a1 / a0;
        self->lp_a2 = a2 / a0;
    }

    // Ganho fixo (ou você pode controlar via outro knob)
    self->b0 = 1.0f;
}

static LV2_Handle instantiate(const LV2_Descriptor* descriptor,
                              double rate, const char* path,
                              const LV2_Feature* const* features) {
    
  Bandpass* self = (Bandpass*)calloc(1, sizeof(Bandpass));
  self->samplerate = (float)rate;
  return (LV2_Handle)self;
}

static void connect_port(LV2_Handle instance, uint32_t port, void* data) {

    Bandpass* self = (Bandpass*)instance;
    switch (port) {
    case 0:
            self->input = (float*)data;
            break;
    case 1:
      self->output = (float*)data;
            break;
    case 2:
	  self->cutoff_control = (float*)data;
	  break;
    case 3:
	self->bypass = (float*)data;
	break;
    case 4:
	self->quality_control = (float*)data;
	break;
    /* case 5: */
    /* 	self->band_gain = (float*)data; */
    /* 	break; */
	
    /* } */
}

static void activate(LV2_Handle instance) {
    /* self->ds_counter = 0; */
    /* self->ds_hold_sample = 0.0f; */

}
static void run(LV2_Handle instance, uint32_t n_samples) {
    Bandpass* self = (Bandpass*)instance;

    if (*(self->bypass) >= 0.05f) {
        for (uint32_t i = 0; i < n_samples; ++i) {
            self->output[i] = self->input[i];
        }
    } else {
        update_filter(self);

        for (uint32_t i = 0; i < n_samples; ++i) {
            float x = self->input[i];

            // High-pass
            float hp = self->hp_b0 * x
                    + self->hp_b1 * self->hp_x1
                    + self->hp_b2 * self->hp_x2
                    - self->hp_a1 * self->hp_y1
                    - self->hp_a2 * self->hp_y2;

            self->hp_x2 = self->hp_x1;
            self->hp_x1 = x;
            self->hp_y2 = self->hp_y1;
            self->hp_y1 = hp;

            // Low-pass
            float lp = self->lp_b0 * hp
                    + self->lp_b1 * self->lp_x1
                    + self->lp_b2 * self->lp_x2
                    - self->lp_a1 * self->lp_y1
                    - self->lp_a2 * self->lp_y2;

            self->lp_x2 = self->lp_x1;
            self->lp_x1 = hp;
            self->lp_y2 = self->lp_y1;
            self->lp_y1 = lp;

            // Downsampling (sample and hold)
            uint32_t ds_factor = 1 + (uint32_t)((1.0f - *(self->quality_control)) * 40.0f);
            
            if (self->ds_counter == 0) {
                self->ds_hold_sample = lp * self->b0;  // aplica ganho aqui
                self->ds_counter = ds_factor;
            } else {
                self->ds_counter--;
            }

            self->output[i] = self->ds_hold_sample;
        }
    }
}

static void deactivate(LV2_Handle instance) {}

static void cleanup(LV2_Handle instance) {
    free(instance);
}

static const LV2_Descriptor descriptor = {
    BANDPASS_URI,
    instantiate,
    connect_port,
    activate,
    run,
    deactivate,
    cleanup,
    NULL
};

LV2_SYMBOL_EXPORT const LV2_Descriptor* lv2_descriptor(uint32_t index) {
    return index == 0 ? &descriptor : NULL;
}
