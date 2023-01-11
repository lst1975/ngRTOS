#ifndef __NGRTOS_PILOT_H__
#define __NGRTOS_PILOT_H__

#define NG_MAX_UP_CHANNEL  4
#define NG_MAX_OUT_CHANNEL 4

struct ng_config
{
  uint32_t flag_up:8;
  uint32_t rover:1;
  uint32_t flag_out:7;
  uint32_t north:1;
  uint32_t east:1;

  /* 
    GGA         Global Positioning System Fix Data
    07626.093, E  ¨C Longitude is in DDDMM.MMMMM format, Longitude 76 deg 26.093,E
    1005.230, N   ¨C Latitude is in DDMM.MMMMM format,  Latitude 10 deg 05.230,N
    
    N, E, W, S represents North, East, West, and South respectively. */
  float longitudes; /* DDDMM.MMMMM */
  float latitudes;  /* DDMM.MMMMM */
  float altitude;

  ng_channel_s *cfgch;
  ng_channel_s *up[NG_MAX_UP_CHANNEL];
  ng_channel_s *out[NG_MAX_OUT_CHANNEL];
};
typedef struct ng_config ng_config_s;

#define NG_UPLOAD_decode  0x80

#define NG_UPLOAD_raw     0x01|NG_UPLOAD_decode
#define NG_UPLOAD_ntrip   0x02|NG_UPLOAD_decode
#define NG_UPLOAD_serial1 0x04
#define NG_UPLOAD_serial2 0x08

#define NG_OUT_serial1    0x01
#define NG_OUT_serial2    0x02

#define NG_DEV_num_out_serial1 0
#define NG_DEV_num_out_serial2 2

ng_config_s *ng_get_sysconfig(void);
void ng_send_outch(ng_config_s *cfg, ng_buffer *b);
void ng_send_upch(ng_config_s *cfg, ng_buffer *b);

#define __ng_send_outch(b) ng_send_outch(ng_get_sysconfig(), b)
#define __ng_send_upch(b) ng_send_upch(ng_get_sysconfig(), b)

#endif
