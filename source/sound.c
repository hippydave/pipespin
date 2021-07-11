//
//  sound.c
//  PipeSpin
//
//  Created by Dave on 23/05/2021.
//

// local headers
#include "sound.h"

// Mixing buffer (globals should go in IWRAM)
// Mixing buffer SHOULD be in IWRAM, otherwise the CPU load
// will _drastially_ increase
u8 myMixingBuffer[ mmMixLen(mmRate) ] __attribute((aligned(4)));

const struct psgSound descend = { 1, 0x007c, 0xd2c0, 0x46e0 | 0x8000 };
const struct psgSound ascend = { 1, 0x0074, 0xd2c0, 0x45d0 | 0x8000 };
const struct psgSound turn1 = { 4, 0x4920, 0x4033 | 0x8000, 0 };
const struct psgSound turn2 = { 4, 0x4920, 0x4034 | 0x8000, 0 };
const struct psgSound turn3 = { 4, 0x0901, 0x4036 | 0x8000, 0 };
const struct psgSound buzz1 = { 4, 0xc210, 0x403c | 0x8000, 0 };
const struct psgSound buzz2 = { 4, 0xc210, 0x403d | 0x8000, 0 };
const struct psgSound levelSound = { 2, 0xf280, 0x0706 | 0x8000, 0 };
const struct psgSound moveTick = { 4, 0x8739, 0x402b | 0x8000, 0 };
const struct psgSound addPipeSound = { 4, 0x661a, 0x4068 | 0x8000, 0 };
const struct psgSound removePipeSound = { 1, 0x0026, 0xf780, 0x4770 | 0x8000 };
const struct psgSound flipSound = { 1, 0x0035, 0xf345, 0x4353 | 0x8000 };
const struct psgSound gameOverSound1 = { 1, 0x007d, 0xf780, 0x0780 | 0x8000 };
const struct psgSound gameOverSound2 = { 4, 0xf700, 0x0056 | 0x8000, 0 };

int musicVolume = 7, sfxVolume = 7;
bool pauseMusicWhenPaused = false;

//---------------------------------------------------------------------------------
void soundEnable ()
//---------------------------------------------------------------------------------
{
    REG_SOUNDCNT_L = DMGSNDCTRL_LVOL(sfxVolume) | DMGSNDCTRL_RVOL(sfxVolume) | DMGSNDCTRL_LSQR1 | DMGSNDCTRL_RSQR1 | DMGSNDCTRL_LNOISE | DMGSNDCTRL_RNOISE | DMGSNDCTRL_LSQR2 | DMGSNDCTRL_RSQR2;
    REG_SOUNDCNT_X = SNDSTAT_ENABLE;
    REG_SOUNDCNT_H = DSOUNDCTRL_DMG100 | DSOUNDCTRL_A100 | DSOUNDCTRL_B100 | DSOUNDCTRL_AL | DSOUNDCTRL_BR;
}//void soundEnable

//---------------------------------------------------------------------------------
void setVolume ()
//---------------------------------------------------------------------------------
{
    REG_SOUNDCNT_L = DMGSNDCTRL_LVOL(sfxVolume) | DMGSNDCTRL_RVOL(sfxVolume) | DMGSNDCTRL_LSQR1 | DMGSNDCTRL_RSQR1 | DMGSNDCTRL_LNOISE | DMGSNDCTRL_RNOISE | DMGSNDCTRL_LSQR2 | DMGSNDCTRL_RSQR2;
    mmSetModuleVolume(musicVolume * 128);
}//void setVolume

//---------------------------------------------------------------------------------
void playSound (struct psgSound sound)
//---------------------------------------------------------------------------------
{
    if (sfxVolume == 0) return;
    soundEnable();
    switch (sound.channel) {
        case 1:
            REG_SOUND1CNT_L = sound.L;
            REG_SOUND1CNT_H = sound.H;
            REG_SOUND1CNT_X = sound.X;
            break;
        case 2:
            REG_SOUND2CNT_L = sound.L;
            REG_SOUND2CNT_H = sound.H;
            break;
        case 3:
            REG_SOUND3CNT_L = sound.L;
            REG_SOUND3CNT_H = sound.H;
            REG_SOUND3CNT_X = sound.X;
            break;
        case 4:
            REG_SOUND4CNT_L = sound.L;
            REG_SOUND4CNT_H = sound.H;
            break;
    }
}//void playSound

//---------------------------------------------------------------------------------
void startSoundMM ()
//---------------------------------------------------------------------------------
{
    u8* myData;
    mm_gba_system mySystem;

    // allocate data for channel buffers & wave buffer (malloc'd data goes to EWRAM)
    // Use the SIZEOF definitions to calculate how many bytes to reserve
    myData = (u8*)malloc( mmNumChans * (MM_SIZEOF_MODCH
                               +MM_SIZEOF_ACTCH
                               +MM_SIZEOF_MIXCH)
                               +mmMixLen(mmRate) );

    // setup system info
    mySystem.mixing_mode       = mmMix(mmRate);

    // number of module/mixing channels
    // higher numbers offer better polyphony at the expense
    // of more memory and/or CPU usage.
    mySystem.mod_channel_count = mmNumChans;
    mySystem.mix_channel_count = mmNumChans;

    // Assign memory blocks to pointers
    mySystem.module_channels   = (mm_addr)(myData+0);
    mySystem.active_channels   = (mm_addr)(myData+(mmNumChans*MM_SIZEOF_MODCH));
    mySystem.mixing_channels   = (mm_addr)(myData+(mmNumChans*(MM_SIZEOF_MODCH
                                                 +MM_SIZEOF_ACTCH)));
    mySystem.mixing_memory     = (mm_addr)myMixingBuffer;
    mySystem.wave_memory       = (mm_addr)(myData+(mmNumChans*(MM_SIZEOF_MODCH
                                                     +MM_SIZEOF_ACTCH
                                                     +MM_SIZEOF_MIXCH)));
    // Pass soundbank address
    mySystem.soundbank         = (mm_addr)soundbank_bin;

    // Initialize Maxmod
    mmInit( &mySystem );

    //initSfx();
}//void startSoundMM
