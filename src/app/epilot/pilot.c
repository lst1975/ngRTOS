
ng_config_s sys_config = {
  .rover       = 1,
  .transparent = 1,
};

ng_config_s *ng_get_sysconfig(void)
{
  return sys_config;
}

void ng_send_outch(ng_config_s *cfg, ng_buffer *b)
{
  int i;
  ng_channel_s *ch;
  
  for (i=0;i<NG_MAX_OUT_CHANNEL;i++)
  {
    ch = cfg->out[i]; 
    if (ch == NULL || ch == cfg->cfgch)
      break;
    (void)ng_channel_putb(ch, b);
  }
}
void ng_send_upch(ng_config_s *cfg, ng_buffer *b)
{
  int i;
  ng_channel_s *ch;
  
  for (i=0;i<NG_MAX_UP_CHANNEL;i++)
  {
    ch = cfg->up[i]; 
    if (ch == NULL || ch == cfg->cfgch)
      break;
    (void)ng_channel_putb(ch, b);
  }
}

