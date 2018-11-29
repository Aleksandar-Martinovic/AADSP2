#define _CRT_SECURE_NO_DEPRECATE

#include <stdlib.h>
#include <string.h>
#include "WAVheader.h"
#include "distortion.h"
#include "float.h"

#define BLOCK_SIZE 16
#define MAX_NUM_CHANNEL 6

double sampleBuffer[MAX_NUM_CHANNEL][BLOCK_SIZE];

/*
L = 0
R = 1
C = 2
Ls = 3
Rs = 4
LFE = 5
*/

void processing(double pInbuf[][BLOCK_SIZE], double pOutbuf[][BLOCK_SIZE]) {

	int i;
	const double input_gain = 0.251189;
	const double headroom_gain = 0.251189;
	const double gain_1 = 0.794328;
	const double gain_2 = 0.630957;
	const double gain_3 = 0.501187;
	const double gain_4 = 0.398107;
	double sum[BLOCK_SIZE];
	double headroom[BLOCK_SIZE];
	distortion_state_t state;

	state.numChannels = MAX_NUM_CHANNEL;
	state.numSamples = BLOCK_SIZE;
	state.gain = headroom_gain;
	state.type = SOFT_CLIPPING;
	state.threshold1 = FLT_MAX;
	state.threshold2 = 3.400000000E+38F;

	for (i = 0; i < BLOCK_SIZE; i++) {
		sum[i] = pInbuf[0][i] * input_gain + pInbuf[1][i] * input_gain;			//Blok +
		headroom[i] = sum[i] * headroom_gain;
		pOutbuf[3][i] = pInbuf[0][i] * input_gain * gain_2;						//Blok Ls
		pOutbuf[4][i] = pInbuf[1][i] * input_gain * gain_1;						//Blok Rs
		pOutbuf[2][i] = headroom[i];											//Blok C
		processSingleChannel(sum, pOutbuf[5], state);							//Blok LFE
		pOutbuf[1][i] = headroom[i] * gain_4;									//Blok R
		pOutbuf[0][i] = headroom[i] * gain_3;									//Blok L

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
	outputWAVhdr.fmt.NumChannels = inputWAVhdr.fmt.NumChannels; // change number of channels

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

			processing(sampleBuffer, sampleBuffer);

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