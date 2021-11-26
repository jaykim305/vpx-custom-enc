#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <map>
#include <string>

#include "./vpxenc_api.h"
#include "./vpx/vpx_codec.h"

using namespace std;

#define LOGGING 1

int g_argc;
char** g_argv;

void set_stream_params(struct VpxEncoderConfig *global,
                               struct stream_state *stream) {
    static const int *ctrl_args_map = vp9_arg_ctrl_map;
    struct stream_config *config = &stream->config;

  // Handle codec specific options
    config->out_fn = g_argv[2];//"/home/jaykim305/vpx-custom-enc/output/mylive_vp9.webm";
    config->write_webm = 1;
    config->cfg.g_threads = 16;
    config->cfg.g_w = 3840;
    config->cfg.g_h = 2160;
    config->cfg.g_error_resilient = 1;
    config->cfg.rc_end_usage = VPX_CBR;
    config->cfg.g_lag_in_frames = 0;
    config->cfg.rc_dropframe_thresh = 0;
    config->cfg.rc_target_bitrate = atoi(g_argv[3]);
    config->cfg.rc_min_quantizer = 4;
    config->cfg.rc_max_quantizer = 48;
    config->cfg.kf_min_dist = 0;
    config->cfg.kf_max_dist = 90;

    // codec ctrls
    set_arg_ctrl(config, ctrl_args_map, VP8E_SET_CPUUSED, 8);
    set_arg_ctrl(config, ctrl_args_map, VP8E_SET_STATIC_THRESHOLD, 0);
    set_arg_ctrl(config, ctrl_args_map, VP9E_SET_TILE_COLUMNS, 4);
    set_arg_ctrl(config, ctrl_args_map, VP9E_SET_FRAME_PARALLEL_DECODING, 1);
    set_arg_ctrl(config, ctrl_args_map, VP9E_SET_ROW_MT, 1);
    set_arg_ctrl(config, ctrl_args_map, VP8E_SET_MAX_INTRA_BITRATE_PCT, 300);
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
    global->framerate.num = 60000; //60fps
    global->framerate.den = 1000;
    global->quiet = 1;
    global->show_psnr = 0;
}

int main(int argc, char *argv[]) {
    std::cout << "This is the custom vpxencoder!" << std::endl;
    std::cout << "Usage: [input] [output] [bitrate]" << std::endl;

    g_argc = argc;
    g_argv = argv;

    vpx_image_t raw;
    int res = 0;
    struct VpxInputContext input;
    struct VpxEncoderConfig global;

    memset(&input, 0, sizeof(input));
    memset(&raw, 0, sizeof(raw));

    // codec configuration
    set_global_config(&global);
    struct stream_state *stream = NULL;
    stream = new_stream(&global, stream);
    set_stream_params(&global, stream);

    // input configuration
    input.filename = argv[1]; //"../Hearthstone.yuv";
    input.fmt = VPX_IMG_FMT_I420;
    open_input_file(&input);
    input.width = stream->config.cfg.g_w;
    input.height = stream->config.cfg.g_h;
    input.bit_depth = VPX_BITS_8;
    set_stream_dimensions(stream, input.width, input.height);
    validate_stream_config(stream, &global);

    if (global.verbose)
      show_stream_config(stream, &global, &input);

    vpx_img_alloc(&raw, input.fmt, input.width, input.height, 32);
    open_output_file(stream, &global, &input.pixel_aspect_ratio);

    initialize_encoder(stream, &global);

    int frames_in = 0;
    uint64_t cx_time = 0;    

    int frame_avail = 1;
    int got_data = 0;

#if LOGGING
    int64_t estimated_time_left = -1;
    int64_t average_rate = -1;
    int64_t lagged_count = 0;
#endif    
    struct vpx_usec_timer total_timer; 
   
    vpx_usec_timer_start(&total_timer);

    while (frame_avail) {
        frame_avail = read_frame(&input, &raw);
        if (frame_avail) frames_in++;
   
#if LOGGING
        struct vpx_usec_timer timer;    
        float fps = usec_to_fps(cx_time, frames_in);
        fprintf(stderr, "\rPass %d/%d ", global.pass+1, global.passes);
        fprintf(stderr, "frame %4d/%-4d %7" PRId64 "B ", frames_in,
            stream->frames_out, (int64_t)stream->nbytes);
        fprintf(stderr, "%7" PRId64 " %s %.2f %s ",
                cx_time > 9999999 ? cx_time / 1000 : cx_time,
                cx_time > 9999999 ? "ms" : "us", fps >= 1.0 ? fps : fps * 60,
                fps >= 1.0 ? "fps" : "fpm");
        print_time("ETA", estimated_time_left);                    
#endif

        got_data = 0;
        get_cx_data(stream, &global, &got_data);

#if LOGGING
        vpx_usec_timer_start(&timer);
#endif
        encode_frame(stream, &global, frame_avail ? &raw : NULL,
                                    frames_in);
#if LOGGING                                    
        vpx_usec_timer_mark(&timer);
        cx_time += vpx_usec_timer_elapsed(&timer);

        if (input.length) {
            int64_t remaining;
            int64_t rate;           
            const int64_t input_pos = ftello(input.file);
            const int64_t input_pos_lagged = input_pos - lagged_count;
            const int64_t limit = input.length;

            rate = cx_time ? input_pos_lagged * (int64_t)1000000 / cx_time : 0;
            remaining = limit - input_pos + lagged_count;        

            average_rate =
                (average_rate <= 0) ? rate : (average_rate * 7 + rate) / 8;
            estimated_time_left = average_rate ? remaining / average_rate : -1;                
        }
#endif        
        fflush(stdout);        
    }

#if LOGGING     
    fprintf(
            stderr,
            "\rframe %4d/%-4d %7" PRId64 "B %7" PRId64 "b/f %7" PRId64
            "b/s %7" PRId64 " %s (%.2f fps)\n",
            frames_in, stream->frames_out,
            (int64_t)stream->nbytes,
            frames_in ? (int64_t)(stream->nbytes * 8 / frames_in) : 0,
            frames_in
                ? (int64_t)stream->nbytes * 8 * (int64_t)global.framerate.num /
                        global.framerate.den / frames_in
                : 0,
            stream->cx_time > 9999999 ? stream->cx_time / 1000 : stream->cx_time,
            stream->cx_time > 9999999 ? "ms" : "us",
            usec_to_fps(stream->cx_time, frames_in));
#endif

    vpx_usec_timer_mark(&total_timer);
    cx_time = vpx_usec_timer_elapsed(&total_timer);
    float fps = usec_to_fps(cx_time, frames_in);
    fprintf(stderr, "\rTotal Tput Summary (read+encode): %7" PRId64 " %s %.2f %s \n",
            cx_time > 9999999 ? cx_time / 1000 : cx_time,
            cx_time > 9999999 ? "ms" : "us", fps >= 1.0 ? fps : fps * 60,
            fps >= 1.0 ? "fps" : "fpm");  
    
    vpx_codec_destroy(&stream->encoder);
    close_input_file(&input);
    close_output_file(stream, global.codec->fourcc);

    vpx_img_free(&raw);
    free(stream);

    return res ? EXIT_FAILURE : EXIT_SUCCESS;
}