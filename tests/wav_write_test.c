/**
 * @file wav_write_test.c
 * @author Takaya Kurosaki 
 * @brief krsynthで合成した音声をwavファイルに出力
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
  krsynth_binary   data;

  int32_t buf_len = OUTPUT_LENGTH;
  int32_t buf_size = sizeof(int16_t) * buf_len;
  int16_t *buf = malloc(buf_size);

  // 金属質な鍵盤っぽい音
  {
      krsynth_binary_set_default(&data);

    data.algorithm = 4;

    data.phase_coarses[0].value = 6;
    data.phase_coarses[1].value = 2;
    data.phase_coarses[2].value = 26;
    data.phase_coarses[3].value = 8;

    for(unsigned i=0; i<KRSYN_NUM_OPERATORS; i++)
    {
      data.envelope_points[1][i] = 192 >> (i&1 ? 0 : 1);
      data.envelope_points[2][i] = 64 >> (i&1 ? 0 : 1);
      data.envelope_points[3][i] = 0 ;

      data.envelope_times[1][i] = 127;
      data.envelope_times[2][i] = 160;
      data.envelope_times[3][i] = 160;
    }

  }

  {
      krtones_bank_binary bankbin ={
          .bank_number = krtones_bank_number_of(0, 0),
          .programs = {
              [0]= &data
          },
      };
      krtones_binary tonebin ={
          .num_banks=1,
          .banks = &bankbin,
    };

      krtones* tones = krtones_new_from_binary(SAMPLING_RATE, &tonebin);

      krsong* song = krsong_new(96, 48*9,
            krsong_events_new(48*9,(krsong_event[]){
                                   [0]={
                                       .num_messages=2,
                                       .messages = krsong_messages_new(2,
                                       (krsong_message[]){
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
                                       .messages = krsong_messages_new(2,
                                       (krsong_message[]){
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
                                       .messages = krsong_messages_new(4,
                                       (krsong_message[]){
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
                                       .messages = krsong_messages_new(2,
                                       (krsong_message[]){
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
                                       .messages = krsong_messages_new(4,
                                       (krsong_message[]){
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
                                       .messages = krsong_messages_new(2,
                                       (krsong_message[]){
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
                                       .messages = krsong_messages_new(4,
                                       (krsong_message[]){
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
                                      .messages = krsong_messages_new(2,
                                      (krsong_message[]){
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

      krsong_state state;
      krsong_state_set_default(&state, tones, SAMPLING_RATE, song->resolution);

      krsong_render(song, SAMPLING_RATE, &state, tones, buf, buf_len);

      krsong_free(song);
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
