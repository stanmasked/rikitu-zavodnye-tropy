#include "rikitu_level_save.h"
#include <math.h>
#include <string.h>
static void put32(uint8_t *p,uint32_t x){
  unsigned j;for(j=0;j<4;j++)p[j]=(uint8_t)(x>>(8u*j));
}
static uint32_t get32(const uint8_t *p){
  return (uint32_t)p[0]|((uint32_t)p[1]<<8)|
         ((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);
}
static uint32_t crc32(const uint8_t *p,size_t n){
  uint32_t v=0xffffffffu;size_t i;unsigned j;
  for(i=0;i<n;i++){
    v^=p[i];
    for(j=0;j<8;j++)v=(v>>1)^((v&1u)?0xedb88320u:0u);
  }
  return ~v;
}
static unsigned ones(unsigned x){
  unsigned k=0;while(x){k+=x&1u;x>>=1;}return k;
}
static int checkpoint_index(const RikituScene *s){
  size_t i;
  if(!isfinite(s->active_checkpoint))return -1;
  if(s->active_checkpoint==105.0f)return 0;
  for(i=0;i<s->checkpoint_total;i++)
    if(s->active_checkpoint==s->checkpoints[i])return (int)i+1;
  return -1;
}
static int shape(const RikituScene *s,const RikituCampaign *c){
  return s&&c&&rikitu_campaign_valid(c)&&
      c->mode==RIKITU_CAMPAIGN_PLAYING&&c->level==s->level_id&&
      s->level_id>=1&&s->level_id<=16&&
      s->coin_total==90&&s->enemy_count==6&&
      s->checkpoint_total==3&&s->lever_count<=RIKITU_MAX_LEVERS&&
      s->max_hearts==c->max_hearts&&
      s->stage.boss_required==(s->level_id%2==0);
}
int rikitu_level_save_encode(uint8_t out[RIKITU_LEVEL_SAVE_BYTES],
                             const RikituScene *s,const RikituLoop *l,
                             const RikituCampaign *c,uint32_t generation){
  uint8_t frag=0,enemy=0,lever=0;
  unsigned coin_count=0,levers_on=0;
  size_t i;int cp;
  if(!out||!l||!generation||!shape(s,c)||
     !isfinite(l->active_seconds)||l->active_seconds<0.0f||
     l->active_seconds>36000.0f||
     s->hearts<1||s->hearts>s->max_hearts||
     s->damage_count<0||s->damage_count>9999||
     s->stage.completed||s->stage.fragment_count<0||
     s->stage.fragment_count>3||s->stage.coin_count<0||
     s->stage.coin_count>90)return 0;
  cp=checkpoint_index(s);
  if(cp<0||s->reached_checkpoints!=cp)return 0;
  memset(out,0,RIKITU_LEVEL_SAVE_BYTES);
  out[0]='R';out[1]='K';out[2]='L';out[3]='3';
  out[4]=3;out[5]=(uint8_t)s->level_id;
  out[6]=(uint8_t)cp;out[7]=(uint8_t)s->hearts;
  out[8]=(uint8_t)s->max_hearts;
  if(s->stage.boss_required){
    if(s->boss.hp<0||s->boss.hp>3||
       s->boss.alive!=(s->boss.hp>0)||
       s->stage.boss_defeated!=(s->boss.hp==0))return 0;
    out[9]=(uint8_t)s->boss.hp;
  }else if(!s->stage.boss_defeated||s->boss.alive||s->boss.hp){
    return 0;
  }
  for(i=0;i<3;i++){
    if(s->stage.fragments[i]>1)return 0;
    if(s->stage.fragments[i])frag|=(uint8_t)(1u<<i);
  }
  if(ones(frag)!=(unsigned)s->stage.fragment_count)return 0;
  out[10]=frag;
  for(i=0;i<6;i++){
    if(s->enemies[i].dead!=0&&s->enemies[i].dead!=1)return 0;
    if(s->enemies[i].dead)enemy|=(uint8_t)(1u<<i);
  }
  out[11]=enemy;
  for(i=0;i<90;i++){
    if(s->stage.coins[i]>1)return 0;
    if(s->stage.coins[i]){
      out[12+i/8]|=(uint8_t)(1u<<(i%8));
      coin_count++;
    }
  }
  if(coin_count!=(unsigned)s->stage.coin_count)return 0;
  for(i=0;i<s->lever_count;i++){
    if(s->levers[i].on!=0&&s->levers[i].on!=1)return 0;
    if(s->levers[i].on){lever|=(uint8_t)(1u<<i);levers_on++;}
  }
  if((int)levers_on!=s->levers_on)return 0;
  put32(out+24,(uint32_t)s->damage_count);
  put32(out+28,(uint32_t)(l->active_seconds*1000.0f+0.5f));
  put32(out+32,generation);
  put32(out+36,(uint32_t)c->wallet);
  out[40]=lever;
  out[41]=(uint8_t)c->purchased_hearts;
  put32(out+60,crc32(out,60));
  return 1;
}
int rikitu_level_save_inspect(const uint8_t *p,size_t size,
                              uint32_t *generation){
  size_t i;
  uint32_t gen;
  if(!p||size!=RIKITU_LEVEL_SAVE_BYTES||
     p[0]!='R'||p[1]!='K'||p[2]!='L'||p[3]!='3'||p[4]!=3||
     p[5]<1||p[5]>16||p[6]>3||p[7]<1||p[8]<3||p[8]>7||
     p[7]>p[8]||p[9]>3||(p[10]&~7u)||(p[11]&~63u)||
     (p[23]&~3u)||(p[40]&~7u)||p[41]>4)return 0;
  for(i=42;i<60;i++)if(p[i])return 0;
  if(crc32(p,60)!=get32(p+60))return 0;
  gen=get32(p+32);
  if(!gen||get32(p+24)>9999u||get32(p+28)>36000000u||
     get32(p+36)>1440u)return 0;
  if(generation)*generation=gen;
  return 1;
}
int rikitu_level_save_decode(RikituScene *s,RikituLoop *l,
                             const RikituCampaign *c,const uint8_t *p,
                             size_t size,uint32_t *generation){
  RikituScene fresh;
  RikituLoop loop;
  uint32_t ms,gen,wallet,damage;
  unsigned frag,enemy,lever,coins=0;
  size_t i;
  unsigned lev_on=0;
  int checkpoint,hp;
  if(!s||!l||!p||size!=RIKITU_LEVEL_SAVE_BYTES||!shape(s,c)||
     s->stage.coin_count||s->stage.fragment_count||
     s->reached_checkpoints||s->damage_count||
     p[0]!='R'||p[1]!='K'||p[2]!='L'||p[3]!='3'||
     p[4]!=3||p[5]!=(uint8_t)c->level||
     p[6]>3||p[7]<1||p[7]>c->max_hearts||
     p[8]!=(uint8_t)c->max_hearts||p[9]>3||
     (p[10]&~7u)||(p[11]&~63u)||(p[23]&~3u)||
     p[41]!=(uint8_t)c->purchased_hearts)return 0;
  for(i=42;i<60;i++)if(p[i])return 0;
  if(get32(p+60)!=crc32(p,60))return 0;
  gen=get32(p+32);ms=get32(p+28);
  wallet=get32(p+36);damage=get32(p+24);
  if(!gen||ms>36000000u||wallet!=(uint32_t)c->wallet||
     damage>9999u)return 0;
  if((p[40]>>s->lever_count)!=0)return 0;
  hp=p[9];
  if(s->stage.boss_required==0 && hp!=0)return 0;
  fresh=*s;
  memset(&loop,0,sizeof(loop));
  frag=p[10];enemy=p[11];lever=p[40];checkpoint=p[6];
  for(i=0;i<3;i++)fresh.stage.fragments[i]=(unsigned char)((frag>>i)&1u);
  fresh.stage.fragment_count=(int)ones(frag);
  for(i=0;i<90;i++){
    fresh.stage.coins[i]=(unsigned char)((p[12+i/8]>>(i%8))&1u);
    coins+=fresh.stage.coins[i];
  }
  fresh.stage.coin_count=(int)coins;
  for(i=0;i<6;i++)fresh.enemies[i].dead=(int)((enemy>>i)&1u);
  for(i=0;i<fresh.lever_count;i++){
    fresh.levers[i].on=(int)((lever>>i)&1u);
    lev_on+=(unsigned)fresh.levers[i].on;
  }
  fresh.levers_on=(int)lev_on;
  if(fresh.stage.boss_required){
    fresh.boss.hp=hp;
    fresh.boss.alive=(hp>0);
    fresh.boss.x=fresh.boss.center;
    fresh.boss.dir=1;
    fresh.boss.hit_seconds=0.0f;
    fresh.stage.boss_defeated=(hp==0);
  }
  fresh.stage.completed=0;
  fresh.max_hearts=(int)p[8];fresh.hearts=(int)p[7];
  fresh.damage_count=(int)damage;
  fresh.reached_checkpoints=checkpoint;
  fresh.active_checkpoint=checkpoint?fresh.checkpoints[checkpoint-1]:105.0f;
  fresh.invulnerability_seconds=1.0f;
  fresh.player.x=fresh.active_checkpoint;
  fresh.player.y=456.0f-fresh.player.h;
  fresh.player.vx=0.0f;fresh.player.vy=0.0f;
  fresh.player.grounded=0;
  loop.player=fresh.player;
  loop.active_seconds=(float)ms/1000.0f;
  *s=fresh;*l=loop;
  if(generation)*generation=gen;
  return 1;
}
