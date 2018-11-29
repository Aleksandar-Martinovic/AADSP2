#pragma once
#include <math.h>
typedef enum {
	HARD_CLIPPING, SOFT_CLIPPING
} clipping_type_t;

typedef struct {
	int numChannels;
	int numSamples;
	int sampleRate;
	clipping_type_t type;
	float gain;
	float threshold1;
	float threshold2;
} distortion_state_t;

void processSingleChannel(double* input, double* output,
	distortion_state_t state);
