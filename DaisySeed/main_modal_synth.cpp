#include "daisy_seed.h"
#include "daisysp.h"
#include "include/Constants.h"
#include "include/filterUtility.h"
#include "include/max98389.h"
#include "lib/DaisySP/Source/Utility/dsp.h"
#include "include/modal.h"
#include "include/RMSController.h"
#include "include/String.h"

#define PRINT_DECIMATION 1024 // Print one value every 512 samples
#define BOOT_RAMP_UP_SECS 8
#define NUM_SYMPETHATICS 17

using namespace daisysp;
using namespace daisy;

DaisySeed hardware;

max98389                    amp;
float                       bootRampUpGain      = 0.f;
float                       bootRampUpIncrement = 0.f;
int                         bootRampUpSamples;
int                         bootRampUpCounter = 0;
Modal                       banjoModes[BANJO_FILTER.numModes];
static RmsTrackerController rmsCtrl(0.4, 20, 0.015, 0.0, 0.0, 0.4f,SAMPLE_RATE);
static StringWaveguide      sympethaticStrings[NUM_SYMPETHATICS];
static BiQuad               highpass;

void ledErrorPulse(int n) {
    for (int i = 0; i < n; i++) {
        hardware.SetLed(true);
        System::Delay(100);
        hardware.SetLed(false);
        System::Delay(100);
    }
}

void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size) {
    bootRampUpGain = fclamp(bootRampUpGain + bootRampUpIncrement, 0.f, 1.f);
    for (size_t i = 0; i < size; i++) {
        float piezoIn = in[0][i];
        piezoIn *= bootRampUpGain;
        piezoIn           = highpass.process(piezoIn);
        float modalOut    = 0.f;
        float softClipped = std::tanh(2.f * piezoIn);
        float stringOut = 0.f;
        for (int str = 0; str < NUM_SYMPETHATICS; str++) {
            stringOut += sympethaticStrings[str].Process((softClipped)) * (1.f / NUM_SYMPETHATICS);
        }
        for (int m = 0; m < BANJO_FILTE R.numModes; m++) {
            modalOut += banjoModes[m].process(1.2f * softClipped);
        }
        modalOut -= 1.2f * softClipped;
        float sum = modalOut + stringOut;
        float gain   = rmsCtrl.processSample(sum);
        float ampOut = gain * sum;
        out[0][i]    = piezoIn;
        out[1][i]    = ampOut;
        out[2][i]    = ampOut;
    }
}


int main(void) {
    hardware.Configure();
    hardware.Init();
    hardware.StartLog(false);

    System::Delay(1000);

    auto err = amp.init(hardware, false);
    if (err != max98389::ERROR_CODE::NO_ERROR) {
        hardware.PrintLine("ERROR: Failed to initialize MAX98389.\n");
        ledErrorPulse(err);
        return -1;
    }

    SaiHandle         external_sai_handle;
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

    if (!external_sai_handle.IsInitialized()) {
        ledErrorPulse(3);
        return -1;
    }

    AudioHandle::Config audio_cfg;
    audio_cfg.blocksize  = 16;
    audio_cfg.samplerate = SaiHandle::Config::SampleRate::SAI_48KHZ;
    audio_cfg.postgain   = 1.f;
    hardware.audio_handle.Init(audio_cfg, hardware.AudioSaiHandle(), external_sai_handle);

    bootRampUpSamples   = BOOT_RAMP_UP_SECS * hardware.AudioSampleRate();
    bootRampUpIncrement = 1.f / (float) bootRampUpSamples * hardware.AudioBlockSize();
    for (int m = 0; m < BANJO_FILTER.numModes; m++) {
        banjoModes[m].init(BANJO_FILTER.model[m].f0, BANJO_FILTER.model[m].Q, SAMPLE_RATE);
    }


    InitSympatheticStrings(sympethaticStrings, NUM_SYMPETHATICS, 110, 0.8, 0.9887f, 0.9998);

    highpass.setCoefficients(-1.9847443991453215, 0.9848598860490441, 0.9924010712985913, -1.9848021425971827,
                             0.9924010712985913);

    hardware.StartAudio(AudioCallback);
    hardware.SetLed(true);

    while (true) {
    }
}
