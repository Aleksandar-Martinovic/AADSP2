#define _CRT_SECURE_NO_DEPRECATE

#include <stdlib.h>
#include <string.h>
#include "WAVheader.h"
#include "distortion.h"

#define BLOCK_SIZE 16
#define MAX_NUM_CHANNEL 6

#define treshold1 0.02500000
#define treshold2 0

enum Enable {OFF, ON};
enum Mode {LR=1, LR_LFE, All, Default};
double sampleBuffer[MAX_NUM_CHANNEL][BLOCK_SIZE];

/*
L = 0
C = 2
R = 1
Ls = 3
Rs = 4
LFE = 5
*/

void processing(double pInbuf[MAX_NUM_CHANNEL][BLOCK_SIZE], double pOutbuf[MAX_NUM_CHANNEL][BLOCK_SIZE], int enable, int mode) 
{

	int i;
	const double input_gain = 0.251189;
	const double gain_1 = 0.794328;
	const double gain_2 = 0.630957;
	const double gain_3 = 0.501187;
	const double gain_4 = 0.398107;
	double sum[BLOCK_SIZE];
	distortion_state_t state;

	state.numChannels = MAX_NUM_CHANNEL;
	state.numSamples = BLOCK_SIZE;
	state.gain = input_gain;
	state.type = HARD_CLIPPING;
	state.threshold1 = treshold1;
	state.threshold2 = treshold2;

	if (enable == ON) 
	{
		if (mode == LR) 
		{
			for (i = 0; i < BLOCK_SIZE; i++) 
			{
				sum[i] = ((pInbuf[0][i] * input_gain) + (pInbuf[1][i] * input_gain)) * input_gain;		//Blok + & Blok Headroom_gain
				pOutbuf[1][i] = sum[i] * gain_4;														//Blok R
				pOutbuf[0][i] = sum[i] * gain_3;														//Blok L
			}
		}

		else if (mode == LR_LFE) 
		{
			for (i = 0; i < BLOCK_SIZE; i++) 
			{
				sum[i] = ((pInbuf[0][i] * input_gain) + (pInbuf[1][i] * input_gain)) * input_gain;		//Blok + & Blok Headroom_gain
				pOutbuf[1][i] = sum[i] * gain_4;														//Blok R
				pOutbuf[0][i] = sum[i] * gain_3;														//Blok L

			}

			processSingleChannel(sum, pOutbuf[5], state);												//Blok LFE
		}

		else if (mode == All) 
		{
			for (i = 0; i < BLOCK_SIZE; i++) 
			{
				sum[i] = ((pInbuf[0][i] * input_gain) + (pInbuf[1][i] * input_gain)) * input_gain;		//Blok + & Blok Headroom_gain
				pOutbuf[3][i] = pInbuf[0][i] * input_gain * gain_2;										//Blok Ls
				pOutbuf[4][i] = pInbuf[1][i] * input_gain * gain_1;										//Blok Rs
				pOutbuf[2][i] = sum[i];																	//Blok C
				pOutbuf[1][i] = sum[i] * gain_4;														//Blok R
				pOutbuf[0][i] = sum[i] * gain_3;														//Blok L

			}

			processSingleChannel(sum, pOutbuf[5], state);												//Blok LFE
		}

		else if (mode == Default) 
		{
			for (i = 0; i < BLOCK_SIZE; i++) 
			{
				sum[i] = ((pInbuf[0][i] * input_gain) + (pInbuf[1][i] * input_gain)) * input_gain;		//Blok + & Blok Headroom_gain
				pOutbuf[3][i] = pInbuf[0][i] * input_gain * gain_2;										//Blok Ls
				pOutbuf[4][i] = pInbuf[1][i] * input_gain * gain_1;										//Blok Rs
				pOutbuf[2][i] = sum[i];																	//Blok C
				pOutbuf[1][i] = sum[i] * gain_4;														//Blok R
				pOutbuf[0][i] = sum[i] * gain_3;														//Blok L
			}
		}

	}

	if (enable == OFF) 
	{	
		//DO NOTHING
	}
}

int main(int argc, char* argv[])
{
	FILE *wav_in = NULL;
	FILE *wav_out = NULL;
	char WavInputName[256];
	char WavOutputName[256];
	WAV_HEADER inputWAVhdr, outputWAVhdr;

	// Init channel buffers
	for (int i = 0; i<MAX_NUM_CHANNEL; i++)
		memset(&sampleBuffer[i], 0, BLOCK_SIZE);

	// Open input and output wav files
	//-------------------------------------------------
	strcpy(WavInputName, argv[1]);
	wav_in = OpenWavFileForRead(WavInputName, "rb");
	strcpy(WavOutputName, argv[2]);
	wav_out = OpenWavFileForRead(WavOutputName, "wb");
	//-------------------------------------------------

	// Read input wav header
	//-------------------------------------------------
	ReadWavHeader(wav_in, inputWAVhdr);
	//-------------------------------------------------

	// Set up output WAV header
	//-------------------------------------------------	
	outputWAVhdr = inputWAVhdr;
	outputWAVhdr.fmt.NumChannels = MAX_NUM_CHANNEL; // change number of channels

	int oneChannelSubChunk2Size = inputWAVhdr.data.SubChunk2Size / inputWAVhdr.fmt.NumChannels;
	int oneChannelByteRate = inputWAVhdr.fmt.ByteRate / inputWAVhdr.fmt.NumChannels;
	int oneChannelBlockAlign = inputWAVhdr.fmt.BlockAlign / inputWAVhdr.fmt.NumChannels;

	outputWAVhdr.data.SubChunk2Size = oneChannelSubChunk2Size*outputWAVhdr.fmt.NumChannels;
	outputWAVhdr.fmt.ByteRate = oneChannelByteRate*outputWAVhdr.fmt.NumChannels;
	outputWAVhdr.fmt.BlockAlign = oneChannelBlockAlign*outputWAVhdr.fmt.NumChannels;


	// Write output WAV header to file
	//-------------------------------------------------
	WriteWavHeader(wav_out, outputWAVhdr);


	// Processing loop
	//-------------------------------------------------	
	{
		int sample;
		int BytesPerSample = inputWAVhdr.fmt.BitsPerSample / 8;
		const double SAMPLE_SCALE = -(double)(1 << 31);		//2^31
		int iNumSamples = inputWAVhdr.data.SubChunk2Size / (inputWAVhdr.fmt.NumChannels*inputWAVhdr.fmt.BitsPerSample / 8);

		// exact file length should be handled correctly...
		for (int i = 0; i<iNumSamples / BLOCK_SIZE; i++)
		{
			for (int j = 0; j<BLOCK_SIZE; j++)
			{
				for (int k = 0; k<inputWAVhdr.fmt.NumChannels; k++)
				{
					sample = 0; //debug
					fread(&sample, BytesPerSample, 1, wav_in);
					sample = sample << (32 - inputWAVhdr.fmt.BitsPerSample); // force signextend
					sampleBuffer[k][j] = sample / SAMPLE_SCALE;				// scale sample to 1.0/-1.0 range		
				}
			}

			int enable = atoi(argv[3]);
			int mode = atoi(argv[4]);

			if (enable < 0 || enable > 1) {
				enable = 1;
			}

			if (mode < 0 || mode > 3) {
				mode = 4;
			}

			processing(sampleBuffer, sampleBuffer, enable, mode);

			for (int j = 0; j<BLOCK_SIZE; j++)
			{
				for (int k = 0; k<outputWAVhdr.fmt.NumChannels; k++)
				{
					sample = sampleBuffer[k][j] * SAMPLE_SCALE;	// crude, non-rounding 			
					sample = sample >> (32 - inputWAVhdr.fmt.BitsPerSample);
					fwrite(&sample, outputWAVhdr.fmt.BitsPerSample / 8, 1, wav_out);
				}
			}
		}
	}

	// Close files
	//-------------------------------------------------	
	fclose(wav_in);
	fclose(wav_out);
	//-------------------------------------------------	

	return 0;
}