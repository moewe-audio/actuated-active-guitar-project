#include <vector>
#include "daisy_seed.h"
#include "daisysp.h"
#include "include/max98389.h"
#include "include/chirpGen.h"
#include "include/filterUtility.h"
#include "lib/libDaisy/src/daisy_seed.h"
#include "lib/libDaisy/src/sys/system.h"

using namespace daisy;
using namespace daisysp;

// ——————————————————————————————————————————————
// Globals
// ——————————————————————————————————————————————
DaisySeed     hw;
max98389      amp;
Chirp         chirp;
FourthOrderFilter* body_filter;


void AudioCallback(AudioHandle::InputBuffer in,
                   AudioHandle::OutputBuffer out,
                   size_t size)
{
    for(size_t i = 0; i < size; i++)
    {
        float piezo_in      = in[0][i];
        float voltage_sense = in[3][i];
        out[0][i] = piezo_in;
        out[1][i] = voltage_sense;
        out[2][i] = chirp.nextSample() * 0.3f;
    }
}

int main(void)
{
    hw.Configure();
    hw.Init();
    hw.StartLog(false);

    System::Delay(1000);

    auto err = amp.init(hw, false);

    SaiHandle external_sai_handle;
    SaiHandle::Config external_sai_cfg{};
    external_sai_cfg.periph          = SaiHandle::Config::Peripheral::SAI_2;
    external_sai_cfg.sr              = SaiHandle::Config::SampleRate::SAI_48KHZ;
    external_sai_cfg.bit_depth       = SaiHandle::Config::BitDepth::SAI_16BIT;
    external_sai_cfg.a_sync          = SaiHandle::Config::Sync::SLAVE;
    external_sai_cfg.a_dir           = SaiHandle::Config::Direction::RECEIVE;
    external_sai_cfg.b_sync          = SaiHandle::Config::Sync::MASTER;
    external_sai_cfg.b_dir           = SaiHandle::Config::Direction::TRANSMIT;
    external_sai_cfg.pin_config.fs   = seed::D27;
    external_sai_cfg.pin_config.mclk = seed::D24;
    external_sai_cfg.pin_config.sck  = seed::D28;
    external_sai_cfg.pin_config.sb   = seed::D25;
    external_sai_cfg.pin_config.sa   = seed::D26;

    /** Initialize the SAI new handle */
    external_sai_handle.Init(external_sai_cfg);

    AudioHandle::Config audio_cfg;
    audio_cfg.blocksize  = 4;
    audio_cfg.samplerate = SaiHandle::Config::SampleRate::SAI_48KHZ;
    audio_cfg.postgain   = 1.f;
    hw.audio_handle.Init(audio_cfg, hw.AudioSaiHandle(), external_sai_handle);
    hw.StartAudio(AudioCallback);
    hw.SetLed(true);
    chirp.init(20.f, 20000.f, 5.f, hw.AudioSampleRate());
    chirp.start();
    hw.SetLed(true);
    while(true)
    {
    }
}
