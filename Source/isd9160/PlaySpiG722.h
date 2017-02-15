#ifndef __PLAYSPIG722_H__
#define __PLAYSPIG722_H__

#define S7DATA_BASE_ADDR_ON_SPI     0

#define PLAYING_CH000               0
#define PLAYING_CH001               1
#define PLAYING_CH010               2
#define PLAYING_CH011               3
#define PLAYING_CH100               4
#define PLAYING_CH101               5
#define PLAYING_CH110               6
#define PLAYING_CH111               7

#define PLAYING_NONE                0
#define PLAYING_ALL                 7

/* Play */
void PlaySpi_PlayVpByChannel(uint16_t vpIdx, uint8_t ch);
void PlaySpi_PlayVpLoopByChannel(uint16_t vpIdx, uint8_t ch, BOOL bLoopPlay);

void PlaySpi_PlaySilenceByChannel(uint8_t ch);
void PlaySpi_PlaySilenceLoopByChannel(uint8_t ch);
void PlaySpi_PlayInsertSilenceByChannel(uint8_t ch);

/* Play Back */
void PlaySpi_PlayBackStopByChannel(uint8_t ch);
void PlaySpi_PlayBackReplayByChannel(uint8_t ch);
void PlaySpi_PlayBackPauseByChannel(uint8_t ch);
void PlaySpi_PlayBackResumeByChannel(uint8_t ch);
void PlaySpi_PlayBackRepeatByChannel(uint8_t ch);
void PlaySpi_PlayBackPlayPauseResumeByChannel(uint8_t ch);
void PlaySpi_PlayBackStopAll(void);
void PlaySpi_PlayBackPauseAll(void);

void PlaySpi_PlayBackCancelLastOne(void);
void PlaySpi_PlayBackClearAll(void);
void PlaySpi_PlayBackCancelAll(void);

/* Volume */
void PlaySpi_VolumeSetByChannel(uint8_t vol, uint8_t ch);
void PlaySpi_VolumeUpByChannel(uint8_t ch);
void PlaySpi_VolumeDownByChannel(uint8_t ch);
void PlaySpi_VolumeMute(void);
void PlaySpi_VolumeUnmute(void);
void PlaySpi_VolumeSetAll(void);
void PlaySpi_VolumeUpAll(void);
void PlaySpi_VolumeDownAll(void);

// Original
void PlaySpi_Close(void);
void PlaySpi_PauseResume(uint8_t flag);
void PlaySpi_PlayLoop(void);
void PlaySpi_StopChannel(uint8_t ch);

void PlaySpi_SetVolume(uint8_t vol);

uint8_t PlaySpi_GetVolume(void);
uint8_t PlaySpi_GetPlayingStatus(void);

#endif /* __PLAYSPIG722_H__ */

/* vim: set ts=4 sw=4 tw=0 list : */
