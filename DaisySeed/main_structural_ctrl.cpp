#include "daisy_seed.h"
#include "daisysp.h"
#include "include/max98389.h"
#include "include/filterUtility.h"

#define PRINT_DECIMATION 1024 // Print one value every 512 samples
#define BOOT_RAMP_UP_SECS 4

using namespace daisysp;
using namespace daisy;

DaisySeed hardware;

max98389 amp;
FourthOrderFilter * bp1;
FourthOrderFilter * bp2;
FourthOrderFilter * bp3;
static size_t sample_counter = 0;
static OnePole bLp, bHp;
float bootRampUpGain = 0.f;
float bootRampUpIncrement = 0.f;
int bootRampUpSamples;
int bootRampUpCounter = 0;

void ledErrorPulse(int n)
{
    for (int i = 0; i < n; i++)
    {
        hardware.SetLed(true);
        System::Delay(100);
        hardware.SetLed(false);
        System::Delay(100);
    }
}

void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size)
{
    float ampOut = 0.0;
    bootRampUpGain = fclamp(bootRampUpGain + bootRampUpIncrement, 0.f, 1.f);
    for(size_t i = 0; i < size; i++)
    {
        const float in_current = in[2][i]; // Assume current readings to be proportional to velocity
        // ampOut = 1.f * bp1->process(in_current) + -1.5f * bp2->process(in_current) +  -1.5f * bp3->process(in_current);
        ampOut = -3.f * bp2->process(in_current);
        ampOut = bLp.Process(ampOut);
        ampOut = bHp.Process(ampOut);
        ampOut *= bootRampUpGain;
        out[0][i] = in_current;
        out[1][i] = ampOut;
        out[2][i] = ampOut;
        out[3][i] = ampOut;
        // if (++sample_counter >= PRINT_DECIMATION)
        // {
        //     sample_counter = 0;
        //     hardware.PrintLine(">velocity:" FLT_FMT(8), FLT_VAR(8,in_current));
        // }
    }
}


int main(void)
{
    hardware.Configure();
    hardware.Init();
    hardware.StartLog(false);

    System::Delay(1000);

    auto err = amp.init(hardware, false);
    if (err != max98389::ERROR_CODE::NO_ERROR)
    {
        hardware.PrintLine("ERROR: Failed to initialize MAX98389.\n");
        ledErrorPulse(err);
        return -1;
    }

    SaiHandle external_sai_handle;
    SaiHandle::Config external_sai_cfg;
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

    if (!external_sai_handle.IsInitialized())
    {
        ledErrorPulse(3);
        return -1;
    }

    AudioHandle::Config audio_cfg;
    audio_cfg.blocksize  = 4;
    audio_cfg.samplerate = SaiHandle::Config::SampleRate::SAI_48KHZ;
    audio_cfg.postgain   = 1.f;
    hardware.audio_handle.Init(audio_cfg, hardware.AudioSaiHandle(), external_sai_handle);


    bp1 = new FourthOrderFilter(hardware.AudioSampleRate());
    bp2 = new FourthOrderFilter(hardware.AudioSampleRate());
    bp3 = new FourthOrderFilter(hardware.AudioSampleRate());
    bp1->setFilterParams(121.f, 20.f);
    bp2->setFilterParams(226.f, 20.f);
    bp3->setFilterParams(565.f, 20.f);

    bLp.Init();
    bLp.SetFilterMode(OnePole::FILTER_MODE_LOW_PASS);
    bLp.SetFrequency(15000.f / hardware.AudioSampleRate()); // onepole impl requires ratio
    bHp.Init();
    bHp.SetFilterMode(OnePole::FILTER_MODE_HIGH_PASS);
    bHp.SetFrequency(40.f / hardware.AudioSampleRate()); // see above

    bootRampUpSamples = BOOT_RAMP_UP_SECS * hardware.AudioSampleRate();
    bootRampUpIncrement = 1.f / (float) bootRampUpSamples * hardware.AudioBlockSize();

    hardware.StartAudio(AudioCallback);
    hardware.SetLed(true);

    while (true) { }
}
