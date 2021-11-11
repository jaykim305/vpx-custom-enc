#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "vpx/vpx_encoder.h"
#include "./tools_common.h"
#include "./vpxenc.h"

void usage_exit(void) {
  //show_help(stderr, 1);
  exit(EXIT_FAILURE);
}

void set_global_config(struct VpxEncoderConfig *global) {
    memset(global, 0, sizeof(*global));
    global->codec = get_vpx_encoder_by_name("vp9");
    global->passes = 1;
    global->pass = 0;
    global->deadline = VPX_DL_REALTIME;
    global->color_type = I420;
    global->verbose = 1;
    global->have_framerate = 1;
    global->framerate.num = 60000;
    global->framerate.den = 1000;
}

int main() {
    std::cout << "This is the custom vpxencoder!" << std::endl;
    vpx_image_t raw;
    struct VpxInputContext input;
    struct VpxEncoderConfig global;
    // struct stream_state *streams = NULL;

    memset(&input, 0, sizeof(input));
    memset(&raw, 0, sizeof(raw));
    set_global_config(&global);

    switch (global.color_type) {
        case I420: input.fmt = VPX_IMG_FMT_I420; break;
        case I422: input.fmt = VPX_IMG_FMT_I422; break;
        case I444: input.fmt = VPX_IMG_FMT_I444; break;
        case I440: input.fmt = VPX_IMG_FMT_I440; break;
        case YV12: input.fmt = VPX_IMG_FMT_YV12; break;
        case NV12: input.fmt = VPX_IMG_FMT_NV12; break;
    }

    // struct stream_state *stream = NULL;
    // stream = new_stream(&global, stream);
    // parse_stream_params(&global, stream, argv)
    return 0;
}