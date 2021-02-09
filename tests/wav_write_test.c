#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include "../krsyn.h"
#include <ksio/serial/binary.h>
#include <ksio/formats/wave.h>

#define SAMPLING_RATE 48000
#define OUTPUT_LENGTH SAMPLING_RATE*4.5;


int main( void )
{

  i32 buf_len = OUTPUT_LENGTH;
  i32 buf_size = sizeof(i16) * buf_len;
  i32 *buf =   malloc(sizeof(i32) * buf_len);
  i16 *writebuf = malloc( buf_size );

  ks_synth_context* ctx = ks_synth_context_new(SAMPLING_RATE);

  {
      ks_tone_list_data tonebin =
        #include "../tools/test_tones/test.kstc"
              ;



      ks_tone_list* tones = ks_tone_list_new_from_data(ctx, &tonebin);

      ks_score_data song = {
          // Magic number : KSCR
          .resolution=48,
          .length=25,
          .data=(ks_score_event[25]){
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
                  .data[0]=64,
                  .data[1]=100,
              },
              {
                  .delta=0,
                  .status=144,
                  .data[0]=72,
                  .data[1]=100,
              },
              {
                  .delta=24,
                  .status=144,
                  .data[0]=74,
                  .data[1]=100,
              },
              {
                  .delta=0,
                  .status=128,
                  .data[0]=72,
                  .data[1]=0,
              },
              {
                  .delta=24,
                  .status=144,
                  .data[0]=76,
                  .data[1]=100,
              },
              {
                  .delta=0,
                  .status=128,
                  .data[0]=74,
                  .data[1]=0,
              },
              {
                  .delta=0,
                  .status=128,
                  .data[0]=64,
                  .data[1]=0,
              },
              {
                  .delta=0,
                  .status=144,
                  .data[0]=62,
                  .data[1]=100,
              },
              {
                  .delta=24,
                  .status=144,
                  .data[0]=77,
                  .data[1]=100,
              },
              {
                  .delta=0,
                  .status=128,
                  .data[0]=76,
                  .data[1]=0,
              },
              {
                  .delta=24,
                  .status=144,
                  .data[0]=76,
                  .data[1]=100,
              },
              {
                  .delta=0,
                  .status=128,
                  .data[0]=77,
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
                  .data[0]=67,
                  .data[1]=100,
              },
              {
                  .delta=24,
                  .status=144,
                  .data[0]=74,
                  .data[1]=100,
              },
              {
                  .delta=0,
                  .status=128,
                  .data[0]=67,
                  .data[1]=0,
              },
              {
                  .delta=24,
                  .status=144,
                  .data[0]=72,
                  .data[1]=100,
              },
              {
                  .delta=0,
                  .status=128,
                  .data[0]=76,
                  .data[1]=0,
              },

              {
                  .delta=0,
                  .status=128,
                  .data[0]=74,
                  .data[1]=0,
              },
              {
                  .delta=0,
                  .status=144,
                  .data[0]=60,
                  .data[1]=100,
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
      ks_score_state_set_default(state, tones, ctx, song.resolution);
      ks_score_data_render(&song, ctx, state, tones, buf, buf_len);

      for(i32 i=0; i<buf_len; i++){
          writebuf[i] = buf[i];
      }

      ks_tone_list_free(tones);
      ks_score_state_free(state);
  }


  {

    ks_wave_file dat;

    dat.chunk_size = sizeof(dat) + 4 - sizeof(u8*) + buf_size;
    dat.fmt_chunk_size = 16;
    dat.audio_format = 1;
    dat.num_channels = 2;
    dat.sampling_freq = SAMPLING_RATE;
    dat.bytes_per_sec = SAMPLING_RATE*4;
    dat.block_size = 2*2;
    dat.bits_per_sample = 16;
    dat.subchunk_size = sizeof(dat) - sizeof(u8*)  + buf_size - 114;
    dat.data = (u8*)writebuf;

    ks_io *io = ks_io_new();
    ks_io_serialize_begin(io, binary_little_endian, dat, ks_wave_file);

    FILE* f = fopen("out.wav", "wb");
    fwrite(io->str->data, 1, io->str->length, f);
    fclose(f);

    ks_io_free(io);
  }

  ks_synth_context_free(ctx);

  free(buf);
  free(writebuf);
}
