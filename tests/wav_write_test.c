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
  uint32_t riff_id;
  uint32_t chunk_size;
  uint32_t format;
  uint32_t format_id;
  uint32_t fmt_chunk_byte_num;
  uint16_t tone_format;
  uint16_t channel_num;
  uint32_t sampling_freq;
  uint32_t mean_byte_num_per_sec;
  uint16_t block_size;
  uint16_t sample_bits;
  uint32_t sub_chunk_id;
  uint32_t sub_chunk_size;
};

int main( void )
{

  int32_t buf_len = OUTPUT_LENGTH;
  int32_t buf_size = sizeof(int16_t) * buf_len;
  int16_t *buf = malloc(buf_size);


  {
      ks_tones_binary tonebin ={
          .num_tones=1,
          .tones=(ks_tone_binary[1]){
              {
                  .msb=0,
                  .lsb=0,
                  .program=0,
                  .note=0,
                  .name="EPiano",
                  .synth={
                      // Magic number : KSYN
                      .phase_coarses={
                          {
                              .u8=56,
                          },
                          {
                              .u8=16,
                          },
                          {
                              .u8=12,
                          },
                          {
                              .u8=4,
                          },
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
                          255,
                          133,
                          255,
                          136,
                      },
                      .envelope_points[2]={
                          255,
                          64,
                          255,
                          64,
                      },
                      .envelope_points[3]={
                          255,
                          0,
                          255,
                          0,
                      },
                      .envelope_times[0]={
                          0,
                          0,
                          0,
                          0,
                      },
                      .envelope_times[1]={
                          255,
                          126,
                          255,
                          126,
                      },
                      .envelope_times[2]={
                          255,
                          156,
                          255,
                          149,
                      },
                      .envelope_times[3]={
                          255,
                          202,
                          255,
                          209,
                      },
                      .envelope_release_times={
                          128,
                          128,
                          128,
                          128,
                      },
                      .velocity_sens={
                          255,
                          255,
                          255,
                          255,
                      },
                      .ratescales={
                          34,
                          186,
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
                          0,
                          0,
                          0,
                          0,
                      },
                      .keyscale_mid_points={
                          69,
                          69,
                          69,
                          69,
                      },
                      .keyscale_curve_types={
                          {
                              .u8=0,
                          },
                          {
                              .u8=0,
                          },
                          {
                              .u8=0,
                          },
                          {
                              .u8=0,
                          },
                      },
                      .lfo_ams_depths={
                          0,
                          0,
                          0,
                          0,
                      },
                      .algorithm=4,
                      .feedback_level=0,
                      .lfo_wave_type=0,
                      .lfo_freq=128,
                      .lfo_det=0,
                      .lfo_fms_depth=0,
                  },
              },
          },
      };

      ks_tones* tones = ks_tones_new_from_binary(SAMPLING_RATE, &tonebin);

      ks_score* song = ks_score_new(96, 48*9,
            ks_score_events_new(48*9,(ks_score_event[]){
                                   [0]={
                                       .num_messages=2,
                                       .messages = ks_score_messages_new(2,
                                       (ks_score_message[]){
                                           [0] ={
                                               .status=0x90,
                                               .datas ={
                                                   [0]=60,
                                                   [1]=100,
                                               },
                                           },
                                          [1] ={
                                              .status=0x90,
                                              .datas ={
                                                  [0]=52,
                                                  [1]=100,
                                              },
                                          },
                                       }
                                       )
                                   },
                                   [96*1/2]={
                                       .num_messages=2,
                                       .messages = ks_score_messages_new(2,
                                       (ks_score_message[]){
                                           [0] ={
                                               .status=0x90,
                                               .datas ={
                                                   [0]=62,
                                                   [1]=100,
                                               },
                                           },
                                          [1] ={
                                              .status=0x80,
                                              .datas ={
                                                  [0]=60,
                                              },
                                          },
                                       }
                                       )
                                   },
                                   [96*2/2]={
                                       .num_messages=4,
                                       .messages = ks_score_messages_new(4,
                                       (ks_score_message[]){
                                           [0] ={
                                               .status=0x90,
                                               .datas ={
                                                   [0]=64,
                                                   [1]=100,
                                               },
                                           },
                                          [1] ={
                                              .status=0x80,
                                              .datas ={
                                                  [0]=62,
                                              },
                                          },
                                          [2] ={
                                              .status=0x80,
                                              .datas ={
                                                  [0]=52,
                                              },
                                          },
                                          [3] ={
                                              .status=0x90,
                                              .datas ={
                                                  [0]=50,
                                                  [1]=100,
                                              },
                                          },
                                       }
                                       )
                                   },
                                   [96*3/2]={
                                       .num_messages=2,
                                       .messages = ks_score_messages_new(2,
                                       (ks_score_message[]){
                                           [0] ={
                                               .status=0x90,
                                               .datas ={
                                                   [0]=65,
                                                   [1]=100,
                                               },
                                           },
                                          [1] ={
                                              .status=0x80,
                                              .datas ={
                                                  [0]=64,
                                              },
                                          },
                                       }
                                       )
                                   },
                                   [96*4/2]={
                                       .num_messages=4,
                                       .messages = ks_score_messages_new(4,
                                       (ks_score_message[]){
                                           [0] ={
                                               .status=0x90,
                                               .datas ={
                                                   [0]=64,
                                                   [1]=100,
                                               },
                                           },
                                          [1] ={
                                              .status=0x80,
                                              .datas ={
                                                  [0]=65,
                                              },
                                          },
                                          [2] ={
                                              .status=0x80,
                                              .datas ={
                                                  [0]=50,
                                              },
                                          },
                                          [3] ={
                                              .status=0x90,
                                              .datas ={
                                                  [0]=55,
                                                  [1]=100,
                                              },
                                          },
                                       }
                                       )
                                   },
                                   [96*5/2]={
                                       .num_messages=2,
                                       .messages = ks_score_messages_new(2,
                                       (ks_score_message[]){
                                           [0] ={
                                               .status=0x90,
                                               .datas ={
                                                   [0]=62,
                                                   [1]=100,
                                               },
                                           },
                                          [1] ={
                                              .status=0x80,
                                              .datas ={
                                                  [0]=55,
                                              },
                                          },
                                       }
                                       )
                                   },
                                   [96*6/2]={
                                       .num_messages=4,
                                       .messages = ks_score_messages_new(4,
                                       (ks_score_message[]){
                                           [0] ={
                                               .status=0x90,
                                               .datas ={
                                                   [0]=60,
                                                   [1]=100,
                                               },
                                           },
                                          [1] ={
                                              .status=0x80,
                                              .datas ={
                                                  [0]=62,
                                              },
                                          },
                                          [2] ={
                                              .status=0x80,
                                              .datas ={
                                                  [0]=62,
                                              },
                                          },
                                          [3] ={
                                              .status=0x90,
                                              .datas ={
                                                  [0]=48,
                                                  [1]=100,
                                              },
                                          },
                                       }
                                       )
                                   },
                                  [96*8/2]={
                                      .num_messages=2,
                                      .messages = ks_score_messages_new(2,
                                      (ks_score_message[]){
                                         [0] ={
                                             .status=0x80,
                                             .datas ={
                                                 [0]=60,
                                             },
                                         },
                                        [1] ={
                                              .status=0x80,
                                              .datas ={
                                                  [0]=48,
                                              },
                                          },
                                      }
                                      )
                                  },
                                   [48*9-1]={
                                       .num_messages=0,
                                   },
                               })
                            );

      ks_score_state* state = ks_score_state_new(4);
      ks_score_state_set_default(state, tones, SAMPLING_RATE, song->resolution);

      ks_score_render(song, SAMPLING_RATE, state, tones, buf, buf_len);

      ks_score_free(song);
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
