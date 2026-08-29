void include_modules(audio_sys_t *self) {
    mixer_t      mixer;
    amp_t        amp_io     [CHANNELS_NUM];
    chorus_t     chorus     [CHANNELS_NUM];
    compressor_t compressor [CHANNELS_NUM];
    delay_t      delay      [CHANNELS_NUM];
    distortion_t distortion [CHANNELS_NUM];
    flanger_t    flanger    [CHANNELS_NUM];
    overdrive_t  overdrive  [CHANNELS_NUM];
    phaser_t     phaser     [CHANNELS_NUM];
    reverb_t     reverb     [CHANNELS_NUM];
    tremolo_t    tremolo    [CHANNELS_NUM];
    vibrato_t    vibrato    [CHANNELS_NUM];

    for (int i = 0; i < CHANNELS_NUM; i++) {
        if (i < CHANNELS_NUM / 2) {
            // assign system in and out modules
            inputModule[i] = amp_io[i];
            outputModule[i] = amp_io[i];
            // default mixer plug in
            mixer->vtable->plug_prev(inputModule[i], i);
            mixer->vtable->plug_next(outputModule[i], i);
        }
        // assign fx to modules array on every channel
        modules[i][M_CHORUS]     = chorus     [i];
        modules[i][M_COMPRESSOR] = compressor [i];
        modules[i][M_DELAY]      = delay      [i];
        modules[i][M_DISTORTION] = distortion [i];
        modules[i][M_FLANGER]    = flanger    [i];
        modules[i][M_OVERDRIVE]  = overdrive  [i];
        modules[i][M_PHASER]     = phaser     [i];
        modules[i][M_REVERB]     = reverb     [i];
        modules[i][M_TREMOLO]    = tremolo    [i];
        modules[i][M_VIBRATO]    = vibrato    [i];
    }
}