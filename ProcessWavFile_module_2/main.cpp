#define _CRT_SECURE_NO_DEPRECATE

#include <stdlib.h>
#include <string.h>
#include "WAVheader.h"
#include "distortion.h"
#include "float.h"
#include "common.h"

#define input_gain FRACT_NUM(0.251189)
#define gain_1 FRACT_NUM(0.794328)
#define gain_2 FRACT_NUM(0.630957)
#define gain_3 FRACT_NUM(0.501187)
#define gain_4 FRACT_NUM(0.398107)

enum Enable { OFF, ON };
enum Mode { LR=1, LR_LFE, All, Default };
DSPfract sampleBuffer[MAX_NUM_CHANNEL][BLOCK_SIZE];

DSPint enable;
DSPint mode;

DSPfract sum[BLOCK_SIZE];

/*
L = 0
R = 1
C = 2
Ls = 3
Rs = 4
LFE = 5
*/

void processing()
{
	DSPfract* p_sum;

	DSPfract* left_Ptr = &sampleBuffer[0][0];
	DSPfract* right_Ptr = left_Ptr + BLOCK_SIZE;
	DSPfract* center_Ptr = right_Ptr + BLOCK_SIZE;
	DSPfract* leftSurround_Ptr = center_Ptr + BLOCK_SIZE;
	DSPfract* rightSurround_Ptr = leftSurround_Ptr + BLOCK_SIZE;
	DSPfract* lowFreqEffects_Ptr = rightSurround_Ptr + BLOCK_SIZE;

	if (enable == ON)
	{
		if (mode == LR)
		{
			for (p_sum = sum; p_sum < sum + BLOCK_SIZE; p_sum++)
			{
				*p_sum = ((*left_Ptr * input_gain) + (*right_Ptr * input_gain));			//Blok + & Blok Headroom_gain
				*p_sum = *p_sum * input_gain;
				*right_Ptr = *p_sum * gain_4;															//Blok R
				*left_Ptr = *p_sum * gain_3;															//Blok L
				right_Ptr++;
				left_Ptr++;
			}

		}

		else if (mode == LR_LFE)
		{
			for (p_sum = sum; p_sum < sum + BLOCK_SIZE; p_sum++)
			{
				*p_sum = ((*left_Ptr * input_gain) + (*right_Ptr * input_gain));			//Blok + & Blok Headroom_gain
				*p_sum = *p_sum * input_gain;
				*right_Ptr = *p_sum * gain_4;															//Blok R
				*left_Ptr = *p_sum * gain_3;															//Blok L
				right_Ptr++;
				left_Ptr++;
			}

			processSingleChannel(sum, lowFreqEffects_Ptr);										//Blok LFE
		}

		else if (mode == All)
		{
			for (p_sum = sum; p_sum < sum + BLOCK_SIZE; p_sum++)
			{
				*p_sum = ((*left_Ptr * input_gain) + (*right_Ptr * input_gain));			//Blok + & Blok Headroom_gain
				*p_sum = *p_sum * input_gain;
				*leftSurround_Ptr = *left_Ptr * input_gain;									//Blok Ls
				*leftSurround_Ptr = *leftSurround_Ptr * gain_2;
				*rightSurround_Ptr = *right_Ptr * input_gain;									//Blok Rs
				*rightSurround_Ptr = *rightSurround_Ptr * gain_1;
				*center_Ptr = *p_sum;																	//Blok C
				*right_Ptr = *p_sum * gain_4;															//Blok R
				*left_Ptr = *p_sum * gain_3;															//Blok L
				leftSurround_Ptr++;
				rightSurround_Ptr++;
				center_Ptr++;
				right_Ptr++;
				left_Ptr++;
			}

			processSingleChannel(sum, lowFreqEffects_Ptr);										//Blok LFE
		}

		else if (mode == Default)
		{
			for (p_sum = sum; p_sum < sum + BLOCK_SIZE; p_sum++)
			{
				*p_sum = ((*left_Ptr * input_gain) + (*right_Ptr * input_gain));			//Blok + & Blok Headroom_gain
				*p_sum = *p_sum * input_gain;
				*leftSurround_Ptr = *left_Ptr * input_gain;									//Blok Ls
				*leftSurround_Ptr = *leftSurround_Ptr * gain_2;
				*rightSurround_Ptr = *right_Ptr * input_gain;									//Blok Rs
				*rightSurround_Ptr = *rightSurround_Ptr * gain_1;
				*center_Ptr = *p_sum;																	//Blok C
				*right_Ptr = *p_sum * gain_4;															//Blok R
				*left_Ptr = *p_sum * gain_3;															//Blok L
				leftSurround_Ptr++;
				rightSurround_Ptr++;
				center_Ptr++;
				right_Ptr++;
				left_Ptr++;
			}

		}

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
	for (DSPint i = 0; i < MAX_NUM_CHANNEL; i++) {
		for (DSPint j = 0; j < BLOCK_SIZE; j++) {
			sampleBuffer[i][j] = FRACT_NUM(0.0);
		}
	}


	// Open input and output wav files
	//-------------------------------------------------
	strcpy(WavInputName, argv[1]);
	wav_in = OpenWavFileForRead(WavInputName, "rb");
	strcpy(WavOutputName, argv[2]);
	wav_out = OpenWavFileForRead(WavOutputName, "wb");
	//-------------------------------------------------

	enable = atoi(argv[3]);
	mode = atoi(argv[4]);

	if (enable < 0 || enable > 1) {
		enable = 1;
	}

	if (mode < 0 || mode > 3) {
		mode = 4;
	}

	// Read input wav header
	//-------------------------------------------------
	ReadWavHeader(wav_in, inputWAVhdr);
	//-------------------------------------------------

	// Set up output WAV header
	//-------------------------------------------------	
	outputWAVhdr = inputWAVhdr;
	outputWAVhdr.fmt.NumChannels = MAX_NUM_CHANNEL; // change number of channels

	DSPint oneChannelSubChunk2Size = inputWAVhdr.data.SubChunk2Size / inputWAVhdr.fmt.NumChannels;
	DSPint oneChannelByteRate = inputWAVhdr.fmt.ByteRate / inputWAVhdr.fmt.NumChannels;
	DSPint oneChannelBlockAlign = inputWAVhdr.fmt.BlockAlign / inputWAVhdr.fmt.NumChannels;

	outputWAVhdr.data.SubChunk2Size = oneChannelSubChunk2Size*outputWAVhdr.fmt.NumChannels;
	outputWAVhdr.fmt.ByteRate = oneChannelByteRate*outputWAVhdr.fmt.NumChannels;
	outputWAVhdr.fmt.BlockAlign = oneChannelBlockAlign*outputWAVhdr.fmt.NumChannels;


	// Write output WAV header to file
	//-------------------------------------------------
	WriteWavHeader(wav_out, outputWAVhdr);


	// Processing loop
	//-------------------------------------------------	
	{
		DSPint sample;
		DSPint BytesPerSample = inputWAVhdr.fmt.BitsPerSample / 8;
		const double SAMPLE_SCALE = -(double)(1 << 31);		//2^31
		DSPint iNumSamples = inputWAVhdr.data.SubChunk2Size / (inputWAVhdr.fmt.NumChannels*inputWAVhdr.fmt.BitsPerSample / 8);

		// exact file length should be handled correctly...
		for (DSPint i = 0; i<iNumSamples / BLOCK_SIZE; i++)
		{
			for (DSPint j = 0; j<BLOCK_SIZE; j++)
			{
				for (DSPint k = 0; k<inputWAVhdr.fmt.NumChannels; k++)
				{
					sample = 0; //debug
					fread(&sample, BytesPerSample, 1, wav_in);
					sample = sample << (32 - inputWAVhdr.fmt.BitsPerSample); // force signextend
					sampleBuffer[k][j] = sample / SAMPLE_SCALE;				// scale sample to 1.0/-1.0 range		
				}
			}



			processing();

			for (DSPint j = 0; j<BLOCK_SIZE; j++)
			{
				for (DSPint k = 0; k<outputWAVhdr.fmt.NumChannels; k++)
				{
					sample = sampleBuffer[k][j].toLong();	// crude, non-rounding 			
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