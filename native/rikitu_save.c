#include "rikitu_save.h"
#include <math.h>
#include <string.h>
#define MAGIC0 'R'
#define MAGIC1 'K'
#define MAGIC2 'S'
#define MAGIC3 '1'
static void u32put(uint8_t *p,uint32_t v){
  unsigned i;for(i=0;i<4;i++)p[i]=(uint8_t)(v>>(i*8));
}
static uint32_t u32get(const uint8_t *p){
  return (uint32_t)p[0]|((uint32_t)p[1]<<8)|
         ((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);
}
static uint32_t crc32(const uint8_t *p,size_t n){
  uint32_t c=0xffffffffu;
  size_t i;unsigned j;
  for(i=0;i<n;i++){
    c^=p[i];
    for(j=0;j<8;j++)c=(c>>1)^((c&1u)?0xedb88320u:0u);
  }
  return ~c;
}
static unsigned bitcount(unsigned m){
  unsigned n=0;while(m){n+=m&1u;m>>=1;}return n;
}
static int checkpoint_index(const RikituScene *s){
  size_t i;
  if(!isfinite(s->active_checkpoint))return -1;
  if(s->active_checkpoint==105.0f)return 0;
  for(i=0;i<s->checkpoint_total;i++)
    if(s->active_checkpoint==s->checkpoints[i])return (int)(i+1);
  return -1;
}
int rikitu_save_encode(uint8_t out[RIKITU_SAVE_BYTES],
                       const RikituScene *s,const RikituLoop *l,
                       uint32_t generation){
  unsigned enemy=0,fragment=0,coin_count=0,i;
  int cp;
  uint32_t ms;
  if(!out||!s||!l||generation==0||
     s->level_id!=1||s->coin_total!=90||s->enemy_count!=6||
     s->checkpoint_total!=3||s->stage.boss_required||
     s->stage.completed<0||s->stage.completed>1||
     s->max_hearts<1||s->max_hearts>RIKITU_MAX_HEARTS||
     s->hearts<1||s->hearts>s->max_hearts||
     s->damage_count<0||s->damage_count>9999||
     !isfinite(l->active_seconds)||l->active_seconds<0.0f||
     l->active_seconds>36000.0f)return 0;
  cp=checkpoint_index(s);
  if(cp<0||s->reached_checkpoints!=cp)return 0;
  memset(out,0,RIKITU_SAVE_BYTES);
  out[0]=MAGIC0;out[1]=MAGIC1;out[2]=MAGIC2;out[3]=MAGIC3;
  out[4]=1;out[5]=1;out[6]=(uint8_t)cp;
  out[7]=(uint8_t)s->hearts;out[8]=(uint8_t)s->max_hearts;
  out[9]=(uint8_t)s->stage.completed;
  for(i=0;i<3;i++){
    if(s->stage.fragments[i]>1)return 0;
    if(s->stage.fragments[i])fragment|=1u<<i;
  }
  if((int)bitcount(fragment)!=s->stage.fragment_count ||
     (s->stage.completed && fragment!=7u))return 0;
  for(i=0;i<6;i++){
    if(s->enemies[i].dead<0||s->enemies[i].dead>1)return 0;
    if(s->enemies[i].dead)enemy|=1u<<i;
  }
  out[10]=(uint8_t)fragment;
  out[11]=(uint8_t)enemy;
  for(i=0;i<90;i++){
    if(s->stage.coins[i]>1)return 0;
    if(s->stage.coins[i]){
      out[12+i/8]|=(uint8_t)(1u<<(i%8));
      coin_count++;
    }
  }
  if((int)coin_count!=s->stage.coin_count)return 0;
  u32put(&out[24],(uint32_t)s->damage_count);
  ms=(uint32_t)(l->active_seconds*1000.0f+0.5f);
  u32put(&out[28],ms);
  u32put(&out[32],generation);
  u32put(&out[40],crc32(out,40));
  return 1;
}
int rikitu_save_decode(RikituScene *s,RikituLoop *l,
                       const uint8_t *p,size_t size,uint32_t *gen){
  RikituScene fresh;
  RikituLoop loop;
  unsigned i,cp,fragments,enemy,coin_count=0;
  uint32_t hits,ms,sequence;
  if(!s||!l||!p||size!=RIKITU_SAVE_BYTES ||
     p[0]!=MAGIC0||p[1]!=MAGIC1||p[2]!=MAGIC2||p[3]!=MAGIC3||
     p[4]!=1||p[5]!=1||p[6]>3||
     p[7]<1||p[8]<1||p[8]>RIKITU_MAX_HEARTS||p[7]>p[8]||
     p[9]>1||(p[10]&~7u)!=0||(p[11]&~63u)!=0||
     (p[23]&~3u)!=0||p[36]||p[37]||p[38]||p[39])return 0;
  if(u32get(&p[40])!=crc32(p,40))return 0;
  hits=u32get(&p[24]);ms=u32get(&p[28]);sequence=u32get(&p[32]);
  if(hits>9999||ms>36000000u||sequence==0)return 0;
  fragments=p[10];enemy=p[11];cp=p[6];
  if(p[9] && fragments!=7u)return 0;
  rikitu_flower_init(&fresh);
  memset(&loop,0,sizeof(loop));
  for(i=0;i<3;i++){
    fresh.stage.fragments[i]=(unsigned char)((fragments>>i)&1u);
  }
  fresh.stage.fragment_count=(int)bitcount(fragments);
  for(i=0;i<90;i++){
    fresh.stage.coins[i]=(unsigned char)((p[12+i/8]>>(i%8))&1u);
    if(fresh.stage.coins[i])coin_count++;
  }
  fresh.stage.coin_count=(int)coin_count;
  for(i=0;i<6;i++)fresh.enemies[i].dead=(int)((enemy>>i)&1u);
  fresh.stage.completed=(int)p[9];
  fresh.max_hearts=(int)p[8];
  fresh.hearts=(int)p[7];
  fresh.damage_count=(int)hits;
  fresh.reached_checkpoints=(int)cp;
  fresh.active_checkpoint=(cp==0)?105.0f:fresh.checkpoints[cp-1];
  fresh.player.x=fresh.active_checkpoint;
  fresh.player.y=456.0f-fresh.player.h;
  fresh.player.vx=0.0f;fresh.player.vy=0.0f;
  fresh.player.grounded=0;
  fresh.invulnerability_seconds=1.0f;
  loop.player=fresh.player;
  loop.active_seconds=(float)ms/1000.0f;
  *s=fresh;*l=loop;
  if(gen)*gen=sequence;
  return 1;
}
