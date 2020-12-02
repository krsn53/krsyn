/**
 * @file wav_write_test.c
 * @author Takaya Kurosaki 
 * @brief ks_synthで合成した音声をwavファイルに出力
 * 参考
 * 音ファイル（拡張子：WAVファイル）のデータ構造について - https://www.youfit.co.jp/archives/1418
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include "../krsyn.h"

#define SAMPLING_RATE 48000
#define OUTPUT_LENGTH SAMPLING_RATE*4.5;

struct wave_header
{
  u32 riff_id;
  u32 chunk_size;
  u32 format;
  u32 format_id;
  u32 fmt_chunk_byte_num;
  u16 tone_format;
  u16 channel_num;
  u32 sampling_freq;
  u32 mean_byte_num_per_sec;
  u16 block_size;
  u16 sample_bits;
  u32 sub_chunk_id;
  u32 sub_chunk_size;
};

int main( void )
{

  i32 buf_len = OUTPUT_LENGTH;
  i32 buf_size = sizeof(i16) * buf_len;
  i16 *buf = malloc(buf_size);


  {
      ks_tone_list_data tonebin ={
          .length=1,
          .data=(ks_tone_data[1]){
                  {
                    .msb=0,
                    .lsb=0,
                    .program=0,
                    .note=0,
                    .name="EPiano",
                    .synth={
                    // Magic number : KSYN
                    .phase_coarses.b={
                        56,
                        16,
                        12,
                        4,
                    },
                    .phase_fines={
                        0,
                        0,
                        0,
                        0,
                    },
                    .phase_dets={
                        0,
                        0,
                        0,
                        0,
                    },
                    .envelope_points[0]={
                        127,
                        255,
                        127,
                        255,
                    },
                    .envelope_points[1]={
                        104,
                        184,
                        104,
                        187,
                    },
                    .envelope_points[2]={
                        0,
                        0,
                        0,
                        0,
                    },
                    .envelope_points[3]={
                        0,
                        0,
                        0,
                        0,
                    },
                    .envelope_times[0]={
                        0,
                        0,
                        0,
                        0,
                    },
                    .envelope_times[1]={
                        142,
                        146,
                        141,
                        150,
                    },
                    .envelope_times[2]={
                        224,
                        224,
                        224,
                        224,
                    },
                    .envelope_times[3]={
                        0,
                        202,
                        0,
                        0,
                    },
                    .envelope_release_times={
                        128,
                        128,
                        128,
                        128,
                    },
                    .velocity_sens={
                        117,
                        255,
                        115,
                        255,
                    },
                    .ratescales={
                        34,
                        183,
                        32,
                        186,
                    },
                    .keyscale_low_depths={
                        0,
                        0,
                        0,
                        0,
                    },
                    .keyscale_high_depths={
                        223,
                        0,
                        203,
                        0,
                    },
                    .keyscale_mid_points={
                        49,
                        69,
                        49,
                        69,
                    },
                    .keyscale_curve_types.b={
                        16,
                        0,
                        16,
                        0,
                    },
                    .lfo_ams_depths={
                        0,
                        0,
                        0,
                        0,
                    },
                    .algorithm=4,
                    .feedback_level=0,
                    .panpot=64,
                    .lfo_wave_type=0,
                    .lfo_freq=128,
                    .lfo_det=0,
                    .lfo_fms_depth=0,
                },
              },
          },
      };



      tone_list* tones = tone_list_new_from_data(SAMPLING_RATE, &tonebin);

      ks_score_data song = {
          // Magic number : KSCR
          .resolution=48,
          .num_events=25,
          .events=(ks_score_event[25]){
              {
                  .delta=0,
                  .status=255,
                  .data[0]=81,
                  .data[1]=128,
                  .data[2]=0,
              },
              {
                  .delta=0,
                  .status=144,
                  .data[0]=60,
                  .data[1]=100,
              },
              {
                  .delta=0,
                  .status=144,
                  .data[0]=52,
                  .data[1]=100,
              },
              {
                  .delta=24,
                  .status=144,
                  .data[0]=62,
                  .data[1]=100,
              },
              {
                  .delta=0,
                  .status=128,
                  .data[0]=60,
                  .data[1]=0,
              },
              {
                  .delta=24,
                  .status=144,
                  .data[0]=64,
                  .data[1]=100,
              },
              {
                  .delta=0,
                  .status=128,
                  .data[0]=62,
                  .data[1]=0,
              },
              {
                  .delta=0,
                  .status=128,
                  .data[0]=52,
                  .data[1]=0,
              },
              {
                  .delta=0,
                  .status=144,
                  .data[0]=50,
                  .data[1]=100,
              },
              {
                  .delta=24,
                  .status=144,
                  .data[0]=65,
                  .data[1]=100,
              },
              {
                  .delta=0,
                  .status=128,
                  .data[0]=64,
                  .data[1]=0,
              },
              {
                  .delta=24,
                  .status=144,
                  .data[0]=64,
                  .data[1]=100,
              },
              {
                  .delta=0,
                  .status=128,
                  .data[0]=65,
                  .data[1]=0,
              },
              {
                  .delta=0,
                  .status=128,
                  .data[0]=50,
                  .data[1]=0,
              },
              {
                  .delta=0,
                  .status=144,
                  .data[0]=55,
                  .data[1]=100,
              },
              {
                  .delta=24,
                  .status=144,
                  .data[0]=62,
                  .data[1]=100,
              },
              {
                  .delta=0,
                  .status=128,
                  .data[0]=55,
                  .data[1]=0,
              },
              {
                  .delta=24,
                  .status=144,
                  .data[0]=60,
                  .data[1]=100,
              },
              {
                  .delta=0,
                  .status=128,
                  .data[0]=64,
                  .data[1]=0,
              },
              {
                  .delta=0,
                  .status=128,
                  .data[0]=62,
                  .data[1]=0,
              },
              {
                  .delta=0,
                  .status=128,
                  .data[0]=62,
                  .data[1]=0,
              },
              {
                  .delta=0,
                  .status=144,
                  .data[0]=48,
                  .data[1]=100,
              },
              {
                  .delta=48,
                  .status=128,
                  .data[0]=60,
                  .data[1]=0,
              },
              {
                  .delta=0,
                  .status=128,
                  .data[0]=48,
                  .data[1]=0,
              },
              {
                  .delta=24,
                  .status=255,
                  .data[0]=47,
                  .data[1]=0,
              },
          },
      };


      ks_score_state* state = ks_score_state_new(4);
      ks_score_state_set_default(state, tones, SAMPLING_RATE, song.resolution);

      ks_score_data_render(&song, SAMPLING_RATE, state, tones, buf, buf_len);

      tone_list_free(tones);
      ks_score_state_free(state);
  }


  {
    FILE* fp = fopen("out.wav", "wb");
    struct wave_header head;
    // 上記サイトでは0x52494646と書かれているが、エンディアンの関係で逆順になっている。
    // リトルエンディアンだと書き込む際、下位のバイトから書き込まれるため、書き込んだものをバイナリエディタで読むと逆になる。
    head.riff_id = 0x46464952;
    head.chunk_size = sizeof(head) + buf_size - 8;
    head.format = 0x45564157;
    head.format_id = 0x20746D66;
    head.fmt_chunk_byte_num = 16;
    head.tone_format = 1;
    head.channel_num = 2;
    head.sampling_freq = SAMPLING_RATE;
    head.mean_byte_num_per_sec = SAMPLING_RATE*2;
    head.block_size = 4;
    head.sample_bits = 16;
    head.sub_chunk_id = 0x61746164;
    head.sub_chunk_size = sizeof(head) + buf_size - 126;

    // リトルエンディアン環境でない場合、以下の書き方だと動かないかもしれない。
    fwrite(&head, 1, sizeof(head), fp);
    fwrite(buf, 1, buf_size, fp);

    fclose(fp);
  }

  free (buf);
}
