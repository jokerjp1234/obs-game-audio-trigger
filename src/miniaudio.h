/*
miniaudio.h - Single file audio playback and capture library for C/C++

This is the miniaudio header file. You need to include this file in your source code
to use miniaudio. For the implementation, you need to define MINIAUDIO_IMPLEMENTATION
in one of your source files before including this header.

Usage:
#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

Note: This is a placeholder for the actual miniaudio.h file.
You should download the actual miniaudio.h from: https://github.com/mackron/miniaudio

For licensing information, see the original miniaudio repository.
*/

#ifndef miniaudio_h
#define miniaudio_h

// This is a placeholder header file
// Please download the actual miniaudio.h from:
// https://github.com/mackron/miniaudio/blob/master/miniaudio.h

#ifdef __cplusplus
extern "C" {
#endif

// Minimal declarations for compilation
typedef int ma_result;
typedef struct ma_engine ma_engine;
typedef struct ma_sound ma_sound;
typedef unsigned char ma_uint8;
typedef unsigned short ma_uint16;
typedef unsigned int ma_uint32;
typedef unsigned long long ma_uint64;
typedef char ma_bool8;

#define MA_TRUE 1
#define MA_FALSE 0

// Result codes
#define MA_SUCCESS 0
#define MA_ERROR -1
#define MA_INVALID_ARGS -2
#define MA_INVALID_OPERATION -3
#define MA_OUT_OF_MEMORY -4
#define MA_OUT_OF_RANGE -5
#define MA_ACCESS_DENIED -6
#define MA_DOES_NOT_EXIST -7
#define MA_ALREADY_EXISTS -8
#define MA_TOO_MANY_OPEN_FILES -9
#define MA_INVALID_FILE -10
#define MA_TOO_BIG -11
#define MA_PATH_TOO_LONG -12
#define MA_NAME_TOO_LONG -13
#define MA_NOT_DIRECTORY -14
#define MA_IS_DIRECTORY -15
#define MA_DIRECTORY_NOT_EMPTY -16
#define MA_AT_END -17
#define MA_NO_SPACE -18
#define MA_BUSY -19
#define MA_IO_ERROR -20
#define MA_INTERRUPT -21
#define MA_UNAVAILABLE -22
#define MA_ALREADY_IN_USE -23
#define MA_BAD_ADDRESS -24
#define MA_BAD_SEEK -25
#define MA_BAD_PIPE -26
#define MA_DEADLOCK -27
#define MA_TOO_MANY_LINKS -28
#define MA_NOT_IMPLEMENTED -29
#define MA_NO_MESSAGE -30
#define MA_BAD_MESSAGE -31
#define MA_NO_DATA_AVAILABLE -32
#define MA_INVALID_DATA -33
#define MA_TIMEOUT -34
#define MA_NO_NETWORK -35
#define MA_NOT_UNIQUE -36
#define MA_NOT_SOCKET -37
#define MA_NO_ADDRESS -38
#define MA_BAD_PROTOCOL -39
#define MA_PROTOCOL_UNAVAILABLE -40
#define MA_PROTOCOL_NOT_SUPPORTED -41
#define MA_PROTOCOL_FAMILY_NOT_SUPPORTED -42
#define MA_ADDRESS_FAMILY_NOT_SUPPORTED -43
#define MA_SOCKET_NOT_SUPPORTED -44
#define MA_CONNECTION_RESET -45
#define MA_ALREADY_CONNECTED -46
#define MA_NOT_CONNECTED -47
#define MA_CONNECTION_REFUSED -48
#define MA_NO_HOST -49
#define MA_IN_PROGRESS -50
#define MA_CANCELLED -51
#define MA_MEMORY_ALREADY_MAPPED -52

// Sound flags
#define MA_SOUND_FLAG_DECODE (1 << 0)
#define MA_SOUND_FLAG_ASYNC (1 << 1)

// Function declarations (these would normally be implemented in the actual miniaudio.h)
ma_result ma_engine_init(const void* config, ma_engine* engine);
void ma_engine_uninit(ma_engine* engine);
ma_result ma_sound_init_from_file(ma_engine* engine, const char* filePath, ma_uint32 flags, void* group, void* fence, ma_sound* sound);
void ma_sound_uninit(ma_sound* sound);
ma_result ma_sound_start(ma_sound* sound);
ma_result ma_sound_stop(ma_sound* sound);
ma_bool8 ma_sound_is_playing(const ma_sound* sound);
void ma_sound_set_volume(ma_sound* sound, float volume);
void ma_sound_set_pitch(ma_sound* sound, float pitch);
void ma_sound_set_looping(ma_sound* sound, ma_bool8 isLooping);
ma_result ma_sound_get_length_in_frames(ma_sound* sound, ma_uint64* length);
ma_result ma_sound_get_cursor_in_pcm_frames(ma_sound* sound, ma_uint64* cursor);
ma_result ma_sound_seek_to_pcm_frame(ma_sound* sound, ma_uint64 frameIndex);
ma_result ma_sound_get_data_format(ma_sound* sound, void* format, ma_uint32* channels, ma_uint32* sampleRate, void* channelMap, size_t channelMapCap);

#ifdef __cplusplus
}
#endif

#endif /* miniaudio_h */

/*
IMPORTANT NOTE:
This is a placeholder file for compilation purposes only.
To use this plugin properly, you MUST download the actual miniaudio.h file from:
https://github.com/mackron/miniaudio/blob/master/miniaudio.h

Replace this file with the actual miniaudio.h for full functionality.
*/