# Implementation notes

In order for the max98389 to work along side daisy seed's onboard codec, the `audio.cpp` provided by libDaisy had to be modified. Therefore, when adding libDaisy make sure to replace the default `audio.cpp` in `lib/libDaisy/src/hid/` with the provided `audio.cpp` in the root folder.

## Details
- Daisy Seed's onboard audio codec uses a 24-bit word size, while the max98389 is configured at 16-bit
    - while the max98389 can be configured at greater word-sizes for the audio output, the current and voltage readings will remain at 16-bit
- when using both codec's (via SAI1 and SAI2) simultaneously, the received data words are all interpreted as 24-bit words
    - thus, the scaling for the external codec is off, leading to weird floating point readings
- the customized `audio.cpp` introduces a botch to treat the channels of the external codec as the 16-bit words they are
    - the same is done for the data send to the codec, all though the symptoms of the issue were not as strong to begin with