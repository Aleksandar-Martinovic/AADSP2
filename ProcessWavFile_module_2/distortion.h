#pragma once
#include <math.h>
#include "common.h"

/*Clipping values for distortion function*/
#define input_gain FRACT_NUM(0.251189)
#define treshold1 FRACT_NUM(0.02500000)
#define treshold2 FRACT_NUM(0.0)

typedef enum {
	HARD_CLIPPING, SOFT_CLIPPING
} clipping_type_t;

typedef struct {
	DSPint numChannels;
	DSPint numSamples;
	DSPint sampleRate;
	clipping_type_t type;
	DSPfract gain;
	DSPfract threshold1;
	DSPfract threshold2;
} distortion_state_t;


void processSingleChannel(DSPfract* input, DSPfract* output);
