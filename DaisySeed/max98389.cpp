#include "daisy_seed.h"
#include "include/max98389.h"

using namespace daisy;

int8_t max98389::init(DaisySeed& hardware, bool prior_reset)
{
    I2CHandle::Config i2c_conf;
    i2c_conf.periph = I2CHandle::Config::Peripheral::I2C_1;
    i2c_conf.speed  = I2CHandle::Config::Speed::I2C_400KHZ;
    i2c_conf.mode   = I2CHandle::Config::Mode::I2C_MASTER;
    i2c_conf.pin_config.scl  = {DSY_GPIOB, 8};
    i2c_conf.pin_config.sda  = {DSY_GPIOB, 9};
    if (device.Init(i2c_conf) != I2CHandle::Result::OK)
    {
        return ERROR_CODE::I2C_PERIPH_ERROR;
    }
    hardware.DelayMs(10);
    if (prior_reset)
    {
        if (!reset(hardware))
        {
            return ERROR_CODE::DEVICE_RESET_ERROR;
        }
        print_device_status(hardware);
    }
    if (!configure())
    {
        return ERROR_CODE::DEVICE_CONFIG_ERROR;
    }
    return ERROR_CODE::NO_ERROR;
}

bool max98389::reset(DaisySeed& hardware)
{
    uint8_t data = 0x01;
    auto result = device.WriteDataAtAddress(slave_address, software_reset_register, 2, &data, 1, TIMEOUT);
    hardware.DelayMs(10);
    return result == I2CHandle::Result::OK;
}


bool max98389::configure()
{
    uint8_t revisionId;
    auto result = device.ReadDataAtAddress(slave_address, revision_id_register, 2, &revisionId, 1, TIMEOUT);
    if (result != I2CHandle::Result::OK || revisionId != expected_revision_id)
    {
        return false;
    }
    uint8_t data = 0b01000000; // 16 bit
    result = device.WriteDataAtAddress(slave_address, pcm_mode_register, 2, &data, 1, TIMEOUT);
    if (result != I2CHandle::Result::OK)
    {
        return false;
    }
    data = 0b00000010; // 32 BCLKS per LRCLK -> 2ch * 16 bits
    result = device.WriteDataAtAddress(slave_address, pcm_clock_register, 2, &data, 1, TIMEOUT);
    if (result != I2CHandle::Result::OK)
    {
        return false;
    }
    data = 0b10001000; // 48 khZ I/V ; 48kHz audio
    result = device.WriteDataAtAddress(slave_address, pcm_sample_rate_register, 2, &data, 1, TIMEOUT);
    if (result != I2CHandle::Result::OK)
    {
        return false;
    }
    data = 0x00;
    result = device.WriteDataAtAddress(slave_address, pcm_imon_slots_register, 2, &data, 1, TIMEOUT);
    if (result != I2CHandle::Result::OK)
    {
        return false;
    }
    data = 0x01;
    result = device.WriteDataAtAddress(slave_address, pcm_vmon_slots_register, 2, &data, 1, TIMEOUT);
    if (result != I2CHandle::Result::OK)
    {
        return false;
    }
    data = 0x03; // enable voltage sense, enable current sense
    result = device.WriteDataAtAddress(slave_address, pcm_tx_source_en_register, 2, &data, 1, TIMEOUT);
    if (result != I2CHandle::Result::OK)
    {
        return false;
    }
    data = 0x01;
    result = device.WriteDataAtAddress(slave_address, pcm_rx_en_register, 2, &data, 1, TIMEOUT);
    if (result != I2CHandle::Result::OK)
    {
        return false;
    }
    data = 0x01;
    result = device.WriteDataAtAddress(slave_address, pcm_tx_en_register, 2, &data, 1, TIMEOUT);
    if (result != I2CHandle::Result::OK)
    {
        return false;
    }
    data = 0b00001010; // Enable thermal auto-recovery and CLOCK_AUTO_RESTART
    result = device.WriteDataAtAddress(slave_address, auto_recovery_register, 2, &data, 1, TIMEOUT);
    if (result != I2CHandle::Result::OK)
    {
        return false;
    }
    data = 0x01;
    result = device.WriteDataAtAddress(slave_address, amp_en_register, 2, &data, 1, TIMEOUT);
    if (result != I2CHandle::Result::OK)
    {
        return false;
    }
    data = 0b00000000; // enable amp ramp up and down
    result = device.WriteDataAtAddress(slave_address, speaker_channel_cfg, 2, &data, 1, TIMEOUT);
    if (result != I2CHandle::Result::OK)
    {
        return false;
    }
    data = 0b0000000; // 0dB digital attenuation
    result = device.WriteDataAtAddress(slave_address, speaker_dig_vol_output_cfg, 2, &data, 1, TIMEOUT);
    if (result != I2CHandle::Result::OK)
    {
        return false;
    }
    data = 0b0011101; // amplfier output: Two-cell mode: 8.41VRMS (+18dB)
    result = device.WriteDataAtAddress(slave_address, speaker_amp_output_cfg, 2, &data, 1, TIMEOUT);
    if (result != I2CHandle::Result::OK)
    {
        return false;
    }
    data = 0x03; // enable current and voltage feedback
    result = device.WriteDataAtAddress(slave_address, iv_data_en_register, 2, &data, 1, TIMEOUT);
    if (result != I2CHandle::Result::OK)
    {
        return false;
    }
    data = 0b00010010; // enable dc blocking filter and dithering
    result = device.WriteDataAtAddress(slave_address, iv_data_dsp_control, 2, &data, 1, TIMEOUT);
    if (result != I2CHandle::Result::OK)
    {
        return false;
    }
    data = 0x00;
    for(uint16_t i = 0; i < 7; i++){
        device.WriteDataAtAddress(slave_address, (uint16_t)(tx_hi_z_control1+i), 2, &data, 1, TIMEOUT);
    }
    data = 0x01; // global enable
    result = device.WriteDataAtAddress(slave_address, global_en_register, 2, &data, 1, TIMEOUT);
    if (result != I2CHandle::Result::OK)
    {
        return false;
    }

    return true;
}


uint8_t max98389::get_device_status(uint16_t status_reg)
{
    uint8_t status;
    auto result = device.ReadDataAtAddress(slave_address, status_reg, 2, &status, 1, TIMEOUT);
    if (result != I2CHandle::Result::OK)
    {
        return 0;
    }
    return status;
}

void max98389::print_device_status(DaisySeed& hardware)
{
    hardware.PrintLine("status 1: %d", get_device_status(read_raw_status_1));
    hardware.PrintLine("status 2: %d", get_device_status(read_raw_status_2));
    hardware.PrintLine("state 1: %d", get_device_status(read_raw_state_1));
    hardware.PrintLine("state 2: %d", get_device_status(read_raw_state_2));
}