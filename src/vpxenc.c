/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "./vpxenc.h"
#include "./vpx_config.h"

#include <assert.h>
#include <limits.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if CONFIG_LIBYUV
#include "third_party/libyuv/include/libyuv/scale.h"
#endif

#include "vpx/vpx_encoder.h"
#if CONFIG_DECODERS
#include "vpx/vpx_decoder.h"
#endif

#include "./args.h"
#include "./ivfenc.h"
#include "./tools_common.h"

#if CONFIG_VP8_ENCODER || CONFIG_VP9_ENCODER
#include "vpx/vp8cx.h"
#endif
#if CONFIG_VP8_DECODER || CONFIG_VP9_DECODER
#include "vpx/vp8dx.h"
#endif

#include "vpx/vpx_integer.h"
#include "vpx_ports/mem_ops.h"
#include "vpx_ports/vpx_timer.h"
#include "./rate_hist.h"
#include "./vpxstats.h"
#include "./warnings.h"
#if CONFIG_WEBM_IO
#include "./webmenc.h"
#endif
#include "./y4minput.h"

static size_t wrap_fwrite(const void *ptr, size_t size, size_t nmemb,
                          FILE *stream) {
  return fwrite(ptr, size, nmemb, stream);
}
#define fwrite wrap_fwrite

static const char *exec_name;

static VPX_TOOLS_FORMAT_PRINTF(3, 0) void warn_or_exit_on_errorv(
    vpx_codec_ctx_t *ctx, int fatal, const char *s, va_list ap) {
  if (ctx->err) {
    const char *detail = vpx_codec_error_detail(ctx);

    vfprintf(stderr, s, ap);
    fprintf(stderr, ": %s\n", vpx_codec_error(ctx));

    if (detail) fprintf(stderr, "    %s\n", detail);

    if (fatal) exit(EXIT_FAILURE);
  }
}

static VPX_TOOLS_FORMAT_PRINTF(2,
                               3) void ctx_exit_on_error(vpx_codec_ctx_t *ctx,
                                                         const char *s, ...) {
  va_list ap;

  va_start(ap, s);
  warn_or_exit_on_errorv(ctx, 1, s, ap);
  va_end(ap);
}

static VPX_TOOLS_FORMAT_PRINTF(3, 4) void warn_or_exit_on_error(
    vpx_codec_ctx_t *ctx, int fatal, const char *s, ...) {
  va_list ap;

  va_start(ap, s);
  warn_or_exit_on_errorv(ctx, fatal, s, ap);
  va_end(ap);
}

size_t FindIndex( const int *a, size_t size, int value )
{
    size_t index = 0;

    while ( index < size && a[index] != value ) ++index;

    return ( index == size ? -1 : index );
}

void usage_exit(void) {
  exit(EXIT_FAILURE);
}

static const int vp8_arg_ctrl_map[] = { VP8E_SET_CPUUSED,
                                        VP8E_SET_ENABLEAUTOALTREF,
                                        VP8E_SET_NOISE_SENSITIVITY,
                                        VP8E_SET_SHARPNESS,
                                        VP8E_SET_STATIC_THRESHOLD,
                                        VP8E_SET_TOKEN_PARTITIONS,
                                        VP8E_SET_ARNR_MAXFRAMES,
                                        VP8E_SET_ARNR_STRENGTH,
                                        VP8E_SET_ARNR_TYPE,
                                        VP8E_SET_TUNING,
                                        VP8E_SET_CQ_LEVEL,
                                        VP8E_SET_MAX_INTRA_BITRATE_PCT,
                                        VP8E_SET_GF_CBR_BOOST_PCT,
                                        VP8E_SET_SCREEN_CONTENT_MODE,
                                        0 };

static const int vp9_arg_ctrl_map[] = { VP8E_SET_CPUUSED,
                                        VP8E_SET_ENABLEAUTOALTREF,
                                        VP8E_SET_SHARPNESS,
                                        VP8E_SET_STATIC_THRESHOLD,
                                        VP9E_SET_TILE_COLUMNS,
                                        VP9E_SET_TILE_ROWS,
                                        VP9E_SET_TPL,
                                        VP8E_SET_ARNR_MAXFRAMES,
                                        VP8E_SET_ARNR_STRENGTH,
                                        VP8E_SET_ARNR_TYPE,
                                        VP8E_SET_TUNING,
                                        VP8E_SET_CQ_LEVEL,
                                        VP8E_SET_MAX_INTRA_BITRATE_PCT,
                                        VP9E_SET_MAX_INTER_BITRATE_PCT,
                                        VP9E_SET_GF_CBR_BOOST_PCT,
                                        VP9E_SET_LOSSLESS,
                                        VP9E_SET_FRAME_PARALLEL_DECODING,
                                        VP9E_SET_AQ_MODE,
                                        VP9E_SET_ALT_REF_AQ,
                                        VP9E_SET_FRAME_PERIODIC_BOOST,
                                        VP9E_SET_NOISE_SENSITIVITY,
                                        VP9E_SET_TUNE_CONTENT,
                                        VP9E_SET_COLOR_SPACE,
                                        VP9E_SET_MIN_GF_INTERVAL,
                                        VP9E_SET_MAX_GF_INTERVAL,
                                        VP9E_SET_TARGET_LEVEL,
                                        VP9E_SET_ROW_MT,
                                        VP9E_SET_DISABLE_LOOPFILTER,
                                        0 };

#define NELEMENTS(x) (sizeof(x) / sizeof(x[0]))
#if CONFIG_VP9_ENCODER
#define ARG_CTRL_CNT_MAX NELEMENTS(vp9_arg_ctrl_map)
#else
#define ARG_CTRL_CNT_MAX NELEMENTS(vp8_arg_ctrl_map)
#endif

/* Per-stream configuration */
struct stream_config {
  struct vpx_codec_enc_cfg cfg;
  const char *out_fn;
  const char *stats_fn;
  stereo_format_t stereo_fmt;
  int arg_ctrls[ARG_CTRL_CNT_MAX][2];
  int arg_ctrl_cnt;
  int write_webm;
};

struct stream_state {
  int index;
  struct stream_state *next;
  struct stream_config config;
  FILE *file;
  struct rate_hist *rate_hist;
  struct WebmOutputContext webm_ctx;
  uint64_t psnr_sse_total;
  uint64_t psnr_samples_total;
  double psnr_totals[4];
  int psnr_count;
  int counts[64];
  vpx_codec_ctx_t encoder;
  unsigned int frames_out;
  uint64_t cx_time;
  size_t nbytes;
  stats_io_t stats;
  struct vpx_image *img;
  vpx_codec_ctx_t decoder;
  int mismatch_seen;
};

static struct stream_state *new_stream(struct VpxEncoderConfig *global,
                                       struct stream_state *prev) {
  struct stream_state *stream;

  stream = calloc(1, sizeof(*stream));
  if (stream == NULL) {
    fatal("Failed to allocate new stream.");
  }

  if (prev) {
    memcpy(stream, prev, sizeof(*stream));
    stream->index++;
    prev->next = stream;
  } else {
    vpx_codec_err_t res;

    /* Populate encoder configuration */
    res = vpx_codec_enc_config_default(global->codec->codec_interface(),
                                       &stream->config.cfg, global->usage);
    if (res) fatal("Failed to get config: %s\n", vpx_codec_err_to_string(res));

    /* Change the default timebase to a high enough value so that the
     * encoder will always create strictly increasing timestamps.
     */
    stream->config.cfg.g_timebase.den = 1000;

    /* Never use the library's default resolution, require it be parsed
     * from the file or set on the command line.
     */
    stream->config.cfg.g_w = 0;
    stream->config.cfg.g_h = 0;

    /* Initialize remaining stream parameters */
    stream->config.write_webm = 1;
#if CONFIG_WEBM_IO
    stream->config.stereo_fmt = STEREO_FORMAT_MONO;
    stream->webm_ctx.last_pts_ns = -1;
    stream->webm_ctx.writer = NULL;
    stream->webm_ctx.segment = NULL;
#endif

    /* Allows removal of the application version from the EBML tags */
    stream->webm_ctx.debug = global->debug;

    /* Default lag_in_frames is 0 in realtime mode CBR mode*/
    if (global->deadline == VPX_DL_REALTIME &&
        stream->config.cfg.rc_end_usage == 1)
      stream->config.cfg.g_lag_in_frames = 0;
  }

  /* Output files must be specified for each stream */
  stream->config.out_fn = NULL;

  stream->next = NULL;
  return stream;
}

static int parse_stream_params(struct VpxEncoderConfig *global,
                               struct stream_state *stream) {
  static const int *ctrl_args_map = NULL;
  struct stream_config *config = &stream->config;
  int eos_mark_found = 0;

  // Handle codec specific options
  if (0) {
#if CONFIG_VP8_ENCODER
  } else if (strcmp(global->codec->name, "vp8") == 0) {
    ctrl_args_map = vp8_arg_ctrl_map;
#endif
#if CONFIG_VP9_ENCODER
  } else if (strcmp(global->codec->name, "vp9") == 0) {
    ctrl_args_map = vp9_arg_ctrl_map;
#endif
  }

  int arg_idx; 
  config->out_fn = "/home/jaykim305/vpx-custom-enc/output/live_vp9.webm";
  config->write_webm = 1;
  config->cfg.g_threads = 8;
  config->cfg.g_w = 1920;
  config->cfg.g_h = 1080;
  config->cfg.g_error_resilient = 1;
  config->cfg.rc_end_usage = VPX_CBR;
  config->cfg.g_lag_in_frames = 0;
  config->cfg.rc_dropframe_thresh = 0;
  config->cfg.rc_target_bitrate = 4000;
  config->cfg.rc_min_quantizer = 4;
  config->cfg.rc_max_quantizer = 48;
  config->cfg.kf_min_dist = 0;
  config->cfg.kf_max_dist = 90;

  arg_idx = FindIndex(vp9_arg_ctrl_map, ARG_CTRL_CNT_MAX, VP8E_SET_CPUUSED);
  config->arg_ctrls[config->arg_ctrl_cnt][0] = ctrl_args_map[arg_idx];
  config->arg_ctrls[config->arg_ctrl_cnt][1] = 6;
  config->arg_ctrl_cnt++;


  arg_idx = FindIndex(vp9_arg_ctrl_map, ARG_CTRL_CNT_MAX, VP8E_SET_STATIC_THRESHOLD);
  config->arg_ctrls[config->arg_ctrl_cnt][0] = ctrl_args_map[arg_idx];
  config->arg_ctrls[config->arg_ctrl_cnt][1] = 0;
  config->arg_ctrl_cnt++;

  arg_idx = FindIndex(vp9_arg_ctrl_map, ARG_CTRL_CNT_MAX, VP9E_SET_TILE_COLUMNS);
  config->arg_ctrls[config->arg_ctrl_cnt][0] = ctrl_args_map[arg_idx];
  config->arg_ctrls[config->arg_ctrl_cnt][1] = 4;
  config->arg_ctrl_cnt++;

  arg_idx = FindIndex(vp9_arg_ctrl_map, ARG_CTRL_CNT_MAX, VP9E_SET_FRAME_PARALLEL_DECODING);
  config->arg_ctrls[config->arg_ctrl_cnt][0] = ctrl_args_map[arg_idx];
  config->arg_ctrls[config->arg_ctrl_cnt][1] = 1;
  config->arg_ctrl_cnt++;

  arg_idx = FindIndex(vp9_arg_ctrl_map, ARG_CTRL_CNT_MAX, VP9E_SET_ROW_MT);
  config->arg_ctrls[config->arg_ctrl_cnt][0] = ctrl_args_map[arg_idx];
  config->arg_ctrls[config->arg_ctrl_cnt][1] = 1;
  config->arg_ctrl_cnt++;

  arg_idx = FindIndex(vp9_arg_ctrl_map, ARG_CTRL_CNT_MAX, VP8E_SET_MAX_INTRA_BITRATE_PCT);
  config->arg_ctrls[config->arg_ctrl_cnt][0] = ctrl_args_map[arg_idx];
  config->arg_ctrls[config->arg_ctrl_cnt][1] = 300;
  config->arg_ctrl_cnt++;

  return eos_mark_found;
}

static void validate_stream_config(const struct stream_state *stream,
                                   const struct VpxEncoderConfig *global) {
  const struct stream_state *streami;
  (void)global;

  if (!stream->config.cfg.g_w || !stream->config.cfg.g_h)
    fatal(
        "Stream %d: Specify stream dimensions with --width (-w) "
        " and --height (-h)",
        stream->index);

  // Check that the codec bit depth is greater than the input bit depth.
  if (stream->config.cfg.g_input_bit_depth >
      (unsigned int)stream->config.cfg.g_bit_depth) {
    fatal("Stream %d: codec bit depth (%d) less than input bit depth (%d)",
          stream->index, (int)stream->config.cfg.g_bit_depth,
          stream->config.cfg.g_input_bit_depth);
  }

  for (streami = stream; streami; streami = streami->next) {
    /* All streams require output files */
    if (!streami->config.out_fn)
      fatal("Stream %d: Output file is required (specify with -o)",
            streami->index);

    /* Check for two streams outputting to the same file */
    if (streami != stream) {
      const char *a = stream->config.out_fn;
      const char *b = streami->config.out_fn;
      if (!strcmp(a, b) && strcmp(a, "/dev/null") && strcmp(a, ":nul"))
        fatal("Stream %d: duplicate output file (from stream %d)",
              streami->index, stream->index);
    }

    /* Check for two streams sharing a stats file. */
    if (streami != stream) {
      const char *a = stream->config.stats_fn;
      const char *b = streami->config.stats_fn;
      if (a && b && !strcmp(a, b))
        fatal("Stream %d: duplicate stats file (from stream %d)",
              streami->index, stream->index);
    }
  }
}

static void set_stream_dimensions(struct stream_state *stream, unsigned int w,
                                  unsigned int h) {
  if (!stream->config.cfg.g_w) {
    if (!stream->config.cfg.g_h)
      stream->config.cfg.g_w = w;
    else
      stream->config.cfg.g_w = w * stream->config.cfg.g_h / h;
  }
  if (!stream->config.cfg.g_h) {
    stream->config.cfg.g_h = h * stream->config.cfg.g_w / w;
  }
}

static const char *file_type_to_string(enum VideoFileType t) {
  switch (t) {
    case FILE_TYPE_RAW: return "RAW";
    case FILE_TYPE_Y4M: return "Y4M";
    default: return "Other";
  }
}

static const char *image_format_to_string(vpx_img_fmt_t f) {
  switch (f) {
    case VPX_IMG_FMT_I420: return "I420";
    case VPX_IMG_FMT_I422: return "I422";
    case VPX_IMG_FMT_I444: return "I444";
    case VPX_IMG_FMT_I440: return "I440";
    case VPX_IMG_FMT_YV12: return "YV12";
    case VPX_IMG_FMT_I42016: return "I42016";
    case VPX_IMG_FMT_I42216: return "I42216";
    case VPX_IMG_FMT_I44416: return "I44416";
    case VPX_IMG_FMT_I44016: return "I44016";
    default: return "Other";
  }
}

static void show_stream_config(struct stream_state *stream,
                               struct VpxEncoderConfig *global,
                               struct VpxInputContext *input) {
#define SHOW(field) \
  fprintf(stderr, "    %-28s = %d\n", #field, stream->config.cfg.field)

  if (stream->index == 0) {
    fprintf(stderr, "Codec: %s\n",
            vpx_codec_iface_name(global->codec->codec_interface()));
    fprintf(stderr, "Source file: %s File Type: %s Format: %s\n",
            input->filename, file_type_to_string(input->file_type),
            image_format_to_string(input->fmt));
  }
  if (stream->next || stream->index)
    fprintf(stderr, "\nStream Index: %d\n", stream->index);
  fprintf(stderr, "Destination file: %s\n", stream->config.out_fn);
  fprintf(stderr, "Encoder parameters:\n");

  SHOW(g_usage);
  SHOW(g_threads);
  SHOW(g_profile);
  SHOW(g_w);
  SHOW(g_h);
  SHOW(g_bit_depth);
  SHOW(g_input_bit_depth);
  SHOW(g_timebase.num);
  SHOW(g_timebase.den);
  SHOW(g_error_resilient);
  SHOW(g_pass);
  SHOW(g_lag_in_frames);
  SHOW(rc_dropframe_thresh);
  SHOW(rc_resize_allowed);
  SHOW(rc_scaled_width);
  SHOW(rc_scaled_height);
  SHOW(rc_resize_up_thresh);
  SHOW(rc_resize_down_thresh);
  SHOW(rc_end_usage);
  SHOW(rc_target_bitrate);
  SHOW(rc_min_quantizer);
  SHOW(rc_max_quantizer);
  SHOW(rc_undershoot_pct);
  SHOW(rc_overshoot_pct);
  SHOW(rc_buf_sz);
  SHOW(rc_buf_initial_sz);
  SHOW(rc_buf_optimal_sz);
  SHOW(rc_2pass_vbr_bias_pct);
  SHOW(rc_2pass_vbr_minsection_pct);
  SHOW(rc_2pass_vbr_maxsection_pct);
  SHOW(rc_2pass_vbr_corpus_complexity);
  SHOW(kf_mode);
  SHOW(kf_min_dist);
  SHOW(kf_max_dist);
  // Temporary use for debug
  SHOW(use_vizier_rc_params);
  SHOW(active_wq_factor.num);
  SHOW(active_wq_factor.den);
}

static void open_output_file(struct stream_state *stream,
                             struct VpxEncoderConfig *global,
                             const struct VpxRational *pixel_aspect_ratio) {
  const char *fn = stream->config.out_fn;
  const struct vpx_codec_enc_cfg *const cfg = &stream->config.cfg;

  if (cfg->g_pass == VPX_RC_FIRST_PASS) return;

  stream->file = strcmp(fn, "-") ? fopen(fn, "wb") : set_binary_mode(stdout);

  if (!stream->file) fatal("Failed to open output file");

  if (stream->config.write_webm && fseek(stream->file, 0, SEEK_CUR))
    fatal("WebM output to pipes not supported.");

#if CONFIG_WEBM_IO
  if (stream->config.write_webm) {
    stream->webm_ctx.stream = stream->file;
    write_webm_file_header(&stream->webm_ctx, cfg, stream->config.stereo_fmt,
                           global->codec->fourcc, pixel_aspect_ratio);
  }
#endif

  if (!stream->config.write_webm) {
    ivf_write_file_header(stream->file, cfg, global->codec->fourcc, 0);
  }
}

static void close_output_file(struct stream_state *stream,
                              unsigned int fourcc) {
  const struct vpx_codec_enc_cfg *const cfg = &stream->config.cfg;

  if (cfg->g_pass == VPX_RC_FIRST_PASS) return;

#if CONFIG_WEBM_IO
  if (stream->config.write_webm) {
    write_webm_file_footer(&stream->webm_ctx);
  }
#endif

  if (!stream->config.write_webm) {
    if (!fseek(stream->file, 0, SEEK_SET))
      ivf_write_file_header(stream->file, &stream->config.cfg, fourcc,
                            stream->frames_out);
  }

  fclose(stream->file);
}

static void setup_pass(struct stream_state *stream,
                       struct VpxEncoderConfig *global, int pass) {
  if (stream->config.stats_fn) {
    if (!stats_open_file(&stream->stats, stream->config.stats_fn, pass))
      fatal("Failed to open statistics store");
  } else {
    if (!stats_open_mem(&stream->stats, pass))
      fatal("Failed to open statistics store");
  }

  stream->config.cfg.g_pass = global->passes == 2
                                  ? pass ? VPX_RC_LAST_PASS : VPX_RC_FIRST_PASS
                                  : VPX_RC_ONE_PASS;
  if (pass) {
    stream->config.cfg.rc_twopass_stats_in = stats_get(&stream->stats);
  }

  stream->cx_time = 0;
  stream->nbytes = 0;
  stream->frames_out = 0;
}

static void initialize_encoder(struct stream_state *stream,
                               struct VpxEncoderConfig *global) {
  int i;
  int flags = 0;

  flags |= global->show_psnr ? VPX_CODEC_USE_PSNR : 0;
  flags |= global->out_part ? VPX_CODEC_USE_OUTPUT_PARTITION : 0;
  /* Construct Encoder Context */
  vpx_codec_enc_init(&stream->encoder, global->codec->codec_interface(),
                     &stream->config.cfg, flags);
  ctx_exit_on_error(&stream->encoder, "Failed to initialize encoder");

  /* Note that we bypass the vpx_codec_control wrapper macro because
   * we're being clever to store the control IDs in an array. Real
   * applications will want to make use of the enumerations directly
   */
  for (i = 0; i < stream->config.arg_ctrl_cnt; i++) {
    int ctrl = stream->config.arg_ctrls[i][0];
    int value = stream->config.arg_ctrls[i][1];
    if (vpx_codec_control_(&stream->encoder, ctrl, value))
      fprintf(stderr, "Error: Tried to set control %d = %d\n", ctrl, value);

    ctx_exit_on_error(&stream->encoder, "Failed to control codec");
  }
}

static void encode_frame(struct stream_state *stream,
                         struct VpxEncoderConfig *global, struct vpx_image *img,
                         unsigned int frames_in) {
  vpx_codec_pts_t frame_start, next_frame_start;
  struct vpx_codec_enc_cfg *cfg = &stream->config.cfg;
  struct vpx_usec_timer timer;

  frame_start =
      (cfg->g_timebase.den * (int64_t)(frames_in - 1) * global->framerate.den) /
      cfg->g_timebase.num / global->framerate.num;
  next_frame_start =
      (cfg->g_timebase.den * (int64_t)(frames_in)*global->framerate.den) /
      cfg->g_timebase.num / global->framerate.num;

  vpx_usec_timer_start(&timer);
  vpx_codec_encode(&stream->encoder, img, frame_start,
                   (unsigned long)(next_frame_start - frame_start), 0,
                   global->deadline);
  vpx_usec_timer_mark(&timer);
  stream->cx_time += vpx_usec_timer_elapsed(&timer);
  ctx_exit_on_error(&stream->encoder, "Stream %d: Failed to encode frame",
                    stream->index);
}

static void update_quantizer_histogram(struct stream_state *stream) {
  if (stream->config.cfg.g_pass != VPX_RC_FIRST_PASS) {
    int q;

    vpx_codec_control(&stream->encoder, VP8E_GET_LAST_QUANTIZER_64, &q);
    ctx_exit_on_error(&stream->encoder, "Failed to read quantizer");
    stream->counts[q]++;
  }
}

static void get_cx_data(struct stream_state *stream,
                        struct VpxEncoderConfig *global, int *got_data) {
  const vpx_codec_cx_pkt_t *pkt;
  const struct vpx_codec_enc_cfg *cfg = &stream->config.cfg;
  vpx_codec_iter_t iter = NULL;

  *got_data = 0;
  while ((pkt = vpx_codec_get_cx_data(&stream->encoder, &iter))) {
    static size_t fsize = 0;
    static FileOffset ivf_header_pos = 0;

    switch (pkt->kind) {
      case VPX_CODEC_CX_FRAME_PKT:
        if (!(pkt->data.frame.flags & VPX_FRAME_IS_FRAGMENT)) {
          stream->frames_out++;
        }
        if (!global->quiet)
          fprintf(stderr, " %6luF", (unsigned long)pkt->data.frame.sz);

        update_rate_histogram(stream->rate_hist, cfg, pkt);
#if CONFIG_WEBM_IO
        if (stream->config.write_webm) {
          write_webm_block(&stream->webm_ctx, cfg, pkt);
        }
#endif
        if (!stream->config.write_webm) {
          if (pkt->data.frame.partition_id <= 0) {
            ivf_header_pos = ftello(stream->file);
            fsize = pkt->data.frame.sz;

            ivf_write_frame_header(stream->file, pkt->data.frame.pts, fsize);
          } else {
            fsize += pkt->data.frame.sz;

            if (!(pkt->data.frame.flags & VPX_FRAME_IS_FRAGMENT)) {
              const FileOffset currpos = ftello(stream->file);
              fseeko(stream->file, ivf_header_pos, SEEK_SET);
              ivf_write_frame_size(stream->file, fsize);
              fseeko(stream->file, currpos, SEEK_SET);
            }
          }

          (void)fwrite(pkt->data.frame.buf, 1, pkt->data.frame.sz,
                       stream->file);
        }
        stream->nbytes += pkt->data.raw.sz;

        *got_data = 1;
        break;
      case VPX_CODEC_STATS_PKT:
        stream->frames_out++;
        stats_write(&stream->stats, pkt->data.twopass_stats.buf,
                    pkt->data.twopass_stats.sz);
        stream->nbytes += pkt->data.raw.sz;
        break;
      case VPX_CODEC_PSNR_PKT:

        if (global->show_psnr) {
          int i;

          stream->psnr_sse_total += pkt->data.psnr.sse[0];
          stream->psnr_samples_total += pkt->data.psnr.samples[0];
          for (i = 0; i < 4; i++) {
            if (!global->quiet)
              fprintf(stderr, "%.3f ", pkt->data.psnr.psnr[i]);
            stream->psnr_totals[i] += pkt->data.psnr.psnr[i];
          }
          stream->psnr_count++;
        }

        break;
      default: break;
    }
  }
}

static float usec_to_fps(uint64_t usec, unsigned int frames) {
  return (float)(usec > 0 ? frames * 1000000.0 / (float)usec : 0);
}

static void print_time(const char *label, int64_t etl) {
  int64_t hours;
  int64_t mins;
  int64_t secs;

  if (etl >= 0) {
    hours = etl / 3600;
    etl -= hours * 3600;
    mins = etl / 60;
    etl -= mins * 60;
    secs = etl;

    fprintf(stderr, "[%3s %2" PRId64 ":%02" PRId64 ":%02" PRId64 "] ", label,
            hours, mins, secs);
  } else {
    fprintf(stderr, "[%3s  unknown] ", label);
  }
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


int main(int argc, const char **argv_) {
  int pass;
  vpx_image_t raw;

  int frame_avail, got_data;

  struct VpxInputContext input;
  struct VpxEncoderConfig global;
  struct stream_state *streams = NULL;
  uint64_t cx_time = 0;
  int stream_cnt = 0;
  int res = 0;

  memset(&input, 0, sizeof(input));
  memset(&raw, 0, sizeof(raw));
  exec_name = argv_[0];

  /* Setup default input stream settings */
  input.framerate.numerator = 30;
  input.framerate.denominator = 1;
  input.only_i420 = 1;
  input.bit_depth = 0;

  /* First parse the global configuration values, because we want to apply
   * other parameters on top of the default configuration provided by the
   * codec.
   */
  set_global_config(&global);

  switch (global.color_type) {
    case I420: input.fmt = VPX_IMG_FMT_I420; break;
    case I422: input.fmt = VPX_IMG_FMT_I422; break;
    case I444: input.fmt = VPX_IMG_FMT_I444; break;
    case I440: input.fmt = VPX_IMG_FMT_I440; break;
    case YV12: input.fmt = VPX_IMG_FMT_YV12; break;
    case NV12: input.fmt = VPX_IMG_FMT_NV12; break;
  }

  /* Now parse each stream's parameters. Using a local scope here
    * due to the use of 'stream' as loop variable in FOREACH_STREAM
    * loops
    */
  struct stream_state *stream = NULL;
  stream = new_stream(&global, stream);
  stream_cnt++;
  if (!streams) streams = stream;
  parse_stream_params(&global, stream);

  input.filename = "../Hearthstone.yuv";

  if (!input.filename) {
    fprintf(stderr, "No input file specified!\n");
    usage_exit();
  }

  /* Decide if other chroma subsamplings than 4:2:0 are supported */
  if (global.codec->fourcc == VP9_FOURCC) input.only_i420 = 0;

  for (pass = global.pass ? global.pass - 1 : 0; pass < global.passes; pass++) {
    int frames_in = 0, seen_frames = 0;
    int64_t estimated_time_left = -1;
    int64_t average_rate = -1;
    int64_t lagged_count = 0;

    open_input_file(&input);

    /* If the input file doesn't specify its w/h (raw files), try to get
     * the data from the first stream's configuration.
     */
    if (!input.width || !input.height) {
        if (stream->config.cfg.g_w && stream->config.cfg.g_h) {
          input.width = stream->config.cfg.g_w;
          input.height = stream->config.cfg.g_h;
          // break;
        }
    }

    /* If input file does not specify bit-depth but input-bit-depth parameter
     * exists, assume that to be the input bit-depth. However, if the
     * input-bit-depth paramter does not exist, assume the input bit-depth
     * to be the same as the codec bit-depth.
     */
    if (!input.bit_depth) {
        if (stream->config.cfg.g_input_bit_depth)
          input.bit_depth = stream->config.cfg.g_input_bit_depth;
        else
          input.bit_depth = stream->config.cfg.g_input_bit_depth =
              (int)stream->config.cfg.g_bit_depth;
      if (input.bit_depth > 8) input.fmt |= VPX_IMG_FMT_HIGHBITDEPTH;
    } else {
      stream->config.cfg.g_input_bit_depth = input.bit_depth;
    }

    set_stream_dimensions(stream, input.width, input.height);
    validate_stream_config(stream, &global);

    /* Show configuration */
    if (global.verbose && pass == 0)
      show_stream_config(stream, &global, &input);

    if (pass == (global.pass ? global.pass - 1 : 0)) {
      // The Y4M reader does its own allocation.
      if (input.file_type != FILE_TYPE_Y4M) {
        vpx_img_alloc(&raw, input.fmt, input.width, input.height, 32);
      }
      stream->rate_hist = init_rate_histogram(
                         &stream->config.cfg, &global.framerate);
    }

    setup_pass(stream, &global, pass);
    open_output_file(stream, &global, &input.pixel_aspect_ratio);
    initialize_encoder(stream, &global);

    frame_avail = 1;
    got_data = 0;

    while (frame_avail || got_data) {
      struct vpx_usec_timer timer;

      if (!global.limit || frames_in < global.limit) {
        frame_avail = read_frame(&input, &raw);

        if (frame_avail) frames_in++;
        seen_frames =
            frames_in > global.skip_frames ? frames_in - global.skip_frames : 0;

        if (!global.quiet) {
          float fps = usec_to_fps(cx_time, seen_frames);
          fprintf(stderr, "\rPass %d/%d ", pass + 1, global.passes);

          if (stream_cnt == 1)
            fprintf(stderr, "frame %4d/%-4d %7" PRId64 "B ", frames_in,
                    streams->frames_out, (int64_t)streams->nbytes);
          else
            fprintf(stderr, "frame %4d ", frames_in);

          fprintf(stderr, "%7" PRId64 " %s %.2f %s ",
                  cx_time > 9999999 ? cx_time / 1000 : cx_time,
                  cx_time > 9999999 ? "ms" : "us", fps >= 1.0 ? fps : fps * 60,
                  fps >= 1.0 ? "fps" : "fpm");
          print_time("ETA", estimated_time_left);
        }

      } else
        frame_avail = 0;

      if (frames_in > global.skip_frames) {

        vpx_usec_timer_start(&timer);
        encode_frame(stream, &global, frame_avail ? &raw : NULL,
                                    frames_in);
        vpx_usec_timer_mark(&timer);
        cx_time += vpx_usec_timer_elapsed(&timer);

        update_quantizer_histogram(stream);

        got_data = 0;
        get_cx_data(stream, &global, &got_data);

        if (!got_data && input.length && streams != NULL &&
            !streams->frames_out) {
          lagged_count = global.limit ? seen_frames : ftello(input.file);
        } else if (input.length) {
          int64_t remaining;
          int64_t rate;

          if (global.limit) {
            const int64_t frame_in_lagged = (seen_frames - lagged_count) * 1000;

            rate = cx_time ? frame_in_lagged * (int64_t)1000000 / cx_time : 0;
            remaining = 1000 * (global.limit - global.skip_frames -
                                seen_frames + lagged_count);
          } else {
            const int64_t input_pos = ftello(input.file);
            const int64_t input_pos_lagged = input_pos - lagged_count;
            const int64_t limit = input.length;

            rate = cx_time ? input_pos_lagged * (int64_t)1000000 / cx_time : 0;
            remaining = limit - input_pos + lagged_count;
          }

          average_rate =
              (average_rate <= 0) ? rate : (average_rate * 7 + rate) / 8;
          estimated_time_left = average_rate ? remaining / average_rate : -1;
        }

      }

      fflush(stdout);
      if (!global.quiet) fprintf(stderr, "\033[K");
    }

    if (!global.quiet) {
      fprintf(
          stderr,
          "\rPass %d/%d frame %4d/%-4d %7" PRId64 "B %7" PRId64 "b/f %7" PRId64
          "b/s %7" PRId64 " %s (%.2f fps)\033[K\n",
          pass + 1, global.passes, frames_in, stream->frames_out,
          (int64_t)stream->nbytes,
          seen_frames ? (int64_t)(stream->nbytes * 8 / seen_frames) : 0,
          seen_frames
              ? (int64_t)stream->nbytes * 8 * (int64_t)global.framerate.num /
                    global.framerate.den / seen_frames
              : 0,
          stream->cx_time > 9999999 ? stream->cx_time / 1000 : stream->cx_time,
          stream->cx_time > 9999999 ? "ms" : "us",
          usec_to_fps(stream->cx_time, seen_frames));
    }

    vpx_codec_destroy(&stream->encoder);
    close_input_file(&input);
    close_output_file(stream, global.codec->fourcc);

    stats_close(&stream->stats, global.passes - 1);

    if (global.pass) break;
  }

  destroy_rate_histogram(stream->rate_hist);


  vpx_img_free(&raw);
  free(streams);
  return res ? EXIT_FAILURE : EXIT_SUCCESS;
}
