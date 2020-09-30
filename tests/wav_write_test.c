/**
 * @file wav_write_test.c
 * @author Takaya Kurosaki 
 * @brief krsynで合成した音声をwavファイルに出力
 * 参考
 * 音ファイル（拡張子：WAVファイル）のデータ構造について - https://www.youfit.co.jp/archives/1418
 */
#include <stdint.h>
#include <stdlib.h>
#include "../krsyn.h"

#define OUTPUT_LENGTH 200000

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
  KrsynCore      *core = krsyn_new(44100);
  KrsynFMData   data;
  KrsynFM        fm;
  KrsynFMNote   note;

  int32_t buf_len = OUTPUT_LENGTH;
  int32_t buf_size = sizeof(int16_t) * buf_len;
  int16_t *buf = malloc(buf_size);

  // 金属質な鍵盤っぽい音
  {
      krsyn_fm_set_data_default(&data);

    data.algorithm = 4;

    data.phase_coarses[0].value = 6;
    data.phase_coarses[1].value = 2;
    data.phase_coarses[2].value = 26;
    data.phase_coarses[3].value = 8;

    for(int i=0; i<KRSYN_NUM_OPERATORS; i++)
    {
      data.envelope_points[1][i] = 192 >> (i&1 ? 0 : 1);
      data.envelope_points[2][i] = 64 >> (i&1 ? 0 : 1);
      data.envelope_points[3][i] = 0 ;

      data.envelope_times[1][i] = 127;
      data.envelope_times[2][i] = 160;
      data.envelope_times[3][i] = 160;
    }

    krsyn_fm_set(core, &fm, &data);
  }

  {
    krsyn_fm_note_on(core, &fm, &note, 60, 100);
    krsyn_fm_render(core, &fm, &note, buf, buf_len/10);
    
    krsyn_fm_note_on(core, &fm, &note, 62, 100);
    krsyn_fm_render(core, &fm, &note, buf+buf_len/10, buf_len/10);

    krsyn_fm_note_on(core, &fm, &note, 64, 100);
    krsyn_fm_render(core, &fm, &note, buf+2*buf_len/10, buf_len/10);
    
    krsyn_fm_note_on(core, &fm, &note, 65, 100);
    krsyn_fm_render(core, &fm, &note, buf+3*buf_len/10, buf_len/10);
    
    krsyn_fm_note_on(core, &fm, &note, 64, 100);
    krsyn_fm_render(core, &fm, &note, buf+4*buf_len/10, buf_len/10);
    
    krsyn_fm_note_on(core, &fm, &note, 62, 100);
    krsyn_fm_render(core, &fm, &note, buf+5*buf_len/10, buf_len/10);
    
    krsyn_fm_note_on(core, &fm, &note, 60, 100);
    krsyn_fm_render(core, &fm, &note, buf+6*buf_len/10, buf_len*3/10);

    krsyn_fm_note_off(&note);
    krsyn_fm_render(core, &fm, &note, buf+9*buf_len/10, buf_len*1/10);
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
    head.sampling_freq = 44100;
    head.mean_byte_num_per_sec = 88200;
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
  krsyn_free(core);
}
