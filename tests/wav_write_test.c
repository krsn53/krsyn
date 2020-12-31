#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include "../krsyn.h"
#include <ksio/formats/wave.h>

#define SAMPLING_RATE 48000
#define OUTPUT_LENGTH SAMPLING_RATE*4.5;


int main( void )
{

  i32 buf_len = OUTPUT_LENGTH;
  i32 buf_size = sizeof(i16) * buf_len;
  i32 *buf =   malloc(sizeof(i32) * buf_len);
  i16 *writebuf = malloc( buf_size );


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
                          4,
                          4,
                          4,
                      },
                      .phase_tunes={
                          127,
                          203,
                          127,
                          127,
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
                          77,
                          160,
                          99,
                          146,
                      },
                      .envelope_points[1]={
                          22,
                          79,
                          0,
                          126,
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
                          105,
                          177,
                          198,
                          130,
                      },
                      .envelope_times[2]={
                          178,
                          195,
                          0,
                          196,
                      },
                      .envelope_times[3]={
                          128,
                          128,
                          128,
                          128,
                      },
                      .velocity_sens={
                          57,
                          255,
                          68,
                          255,
                      },
                      .ratescales={
                          123,
                          116,
                          110,
                          108,
                      },
                      .keyscale_low_depths={
                          103,
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
                          49,
                          0,
                          0,
                          0,
                      },
                      .keyscale_curve_types.b={
                          2,
                          0,
                          0,
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



      ks_tone_list* tones = ks_tone_list_new_from_data(SAMPLING_RATE, &tonebin);

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
      ks_score_state_set_default(state, tones, SAMPLING_RATE, song.resolution);

      ks_score_data_render(&song, SAMPLING_RATE, state, tones, buf, buf_len);

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
    ks_io_begin_serialize(io, binary_little_endian, ks_prop_root(dat, ks_wave_file));

    FILE* f = fopen("out.wav", "wb");
    fwrite(io->str->data, 1, io->str->length, f);
    fclose(f);

    ks_io_free(io);
  }

  free (buf);
  free(writebuf);
}
