#include <stdio.h>
#include <stdlib.h>
#include <vorbis/vorbisfile.h>

/* The number of bytes to get from the codec in each run */
#define CODEC_TRANSFER_SIZE 4096
short mainBuffer[CODEC_TRANSFER_SIZE];

/** SampleFormat.h from Audacity **/
typedef enum {
   int16Sample = 0x00020001,
   int24Sample = 0x00040001,
   floatSample = 0x0004000F
} sampleFormat;

/** Return the size (in memory) of one sample (bytes) */
#define SAMPLE_SIZE(SampleFormat) (SampleFormat >> 16)

// the above mess works out to 2 for int16Sample

/**********************/


// structure to hold the output from a single channel in a named file
typedef struct {
    int size;
    FILE *handle;
    char name[256];
} channelEntry;

// a crude WAV header
typedef struct {
    char riff[4];        // "RIFF"
    int chunkSize;       // total file size - 8 bytes
    char format[4];      // "WAVE"
    char subChunk1Id[4]; // "fmt "
    int subChunk1Size;   // 16 for PCM
    short audioFormat;   // PCM = 1, or other junk if compressed
    short numChannels;   // number of channels
    int sampleRate;      // 44100, 22050, etc.
    int byteRate;        // SampleRate * NumChannels * BitsPerSample / 8
    short blockAlign;    // NumChannels * BitsPerSample / 8
    short bitsPerSample; // 8 bit, 16 bit, etc.
    char subChunk2Id[4]; // "data"
    int subChunk2Size;   // chunkSize - 36
} waveHeader;

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <ogg file>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    OggVorbis_File vorbisFile;    
    OggVorbis_File *mVorbisFile = &vorbisFile;
    int openResult;
    int channels;
    long totalSamples;
    long sampleRate;
    int c, i;
    int sampleSize = SAMPLE_SIZE(int16Sample); // 2

    openResult = ov_fopen(argv[1], mVorbisFile);
    char **commentPointer = ov_comment(mVorbisFile, -1)->user_comments;
    vorbis_info *oggInfo = ov_info(mVorbisFile, -1);
    
    // print comments
    while (*commentPointer) {
        fprintf(stderr, "%s\n", *commentPointer);
        ++commentPointer;
    }
    
    channels = oggInfo->channels;
    sampleRate = oggInfo->rate;
    totalSamples = ov_pcm_total(mVorbisFile, -1);
    
    fprintf(stderr, "Channel: %d (%ldHz)\n", channels, sampleRate);
    fprintf(stderr, "Samples: %ld\n", totalSamples);
    fprintf(stderr, "Length: %.3f\n", ov_time_total(mVorbisFile, -1));
    fprintf(stderr, "Encoder: %s\n\n", ov_comment(mVorbisFile, -1)->vendor);

    // generate a reusable WAV header, with chunk sizes at 0, to be filled in later
    waveHeader w = {"RIFF", 0, "WAVE", "fmt ", 16, 1, 1, sampleRate, sampleRate * 1 * 16 / 8, 1 * 16 / 8, 16, "data", 0};

    // create wav files for each channel
    channelEntry channelEntries[channels];
    int channelIndex;
    for (channelIndex=0; channelIndex < channels; channelIndex++) {
        channelEntries[channelIndex].size = 0;
        sprintf(channelEntries[channelIndex].name, "%d.wav", channelIndex);
        channelEntries[channelIndex].handle = fopen(channelEntries[channelIndex].name, "wb");
        fwrite(&w, sizeof(waveHeader), 1, channelEntries[channelIndex].handle);
    }

    // sniping stuff from Audacity here...

    // seek to the beginning, just in case
    ov_pcm_seek(mVorbisFile, 0);
    
    long bytesRead = 0;
    long samplesRead = 0;
    int bitstream = 0;
    int samplesSinceLastCallback = 0;
    int endian = 0; // forcing little endian
    int totalReadSamples = 0;

    do {
        /* get data from the decoder */
        bytesRead = ov_read(mVorbisFile, (char *) mainBuffer,
                            CODEC_TRANSFER_SIZE,
                            endian,
                            2,    // word length (2 for 16 bit samples)
                            1,    // signed
                            &bitstream);
        if (bytesRead < 0) {
            /* Malformed Ogg Vorbis file. */
            /* TODO: Return some sort of meaningful error. */
            fprintf(stderr, "Malformed Ogg Vorbis file.");
            break;
        }

        // compute sample size based on bytes read and the number of channels
        samplesRead = bytesRead / mVorbisFile->vi[bitstream].channels / sizeof(short);
        totalReadSamples += samplesRead;

        /* give the data to the wavetracks */
        // based on the stride, append every nth 16-bit (2 byte) piece of data
        for (c = 0; c < mVorbisFile->vi[bitstream].channels; c++)
        {
            for(i = 0; i < bytesRead/sizeof(short); i += mVorbisFile->vi[bitstream].channels)
            {
                fwrite((char *)(mainBuffer + c + i), sampleSize, 1, channelEntries[c].handle);
                channelEntries[c].size += sampleSize;
            }
        }

/*
            // the above loops are replicating this, more or less (from Audacity)
            for (c = 0; c < mVorbisFile->vi[bitstream].channels; c++)
                mChannels[bitstream][c]->Append((char *)(mainBuffer + c),
                int16Sample,                          // format
                samplesRead,                          // len
                mVorbisFile->vi[bitstream].channels); // stride, see WaveClip.cpp
*/

    } while (bytesRead != 0);

    // seek to beginning and write size out WAV header
    for (c = 0; c < channels; c++)
    {
        w.chunkSize = channelEntries[c].size + 36;
        w.subChunk2Size = channelEntries[c].size;
        rewind(channelEntries[c].handle);
        fwrite(&w, sizeof(waveHeader), 1, channelEntries[c].handle);
        fclose(channelEntries[c].handle);
    }

    fprintf(stderr, "Total read samples: %d\n", totalReadSamples);
    ov_clear(mVorbisFile);
}
