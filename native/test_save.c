#include "rikitu_save.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#define FILE_A "/tmp/rikitu-map01-save-test-a.rks"
#define FILE_B "/tmp/rikitu-map01-save-test-b.rks"
#define WRONG_A "/tmp/rikitu-map01-save-test-wrong-a.rkc"
#define WRONG_B "/tmp/rikitu-map01-save-test-wrong-b.rkc"
static void write_bytes(const char *path,const char *bytes,size_t n){
  FILE *f=fopen(path,"wb");assert(f);
  assert(fwrite(bytes,1,n,f)==n&&fclose(f)==0);
}
static void assert_bytes(const char *path,const char *expected,size_t n){
  char actual[16];FILE *f=fopen(path,"rb");assert(f&&n<=sizeof(actual));
  assert(fread(actual,1,n,f)==n&&fgetc(f)==EOF&&fclose(f)==0);
  assert(memcmp(actual,expected,n)==0);
}
static void init(RikituScene *s,RikituLoop *l){
  rikitu_flower_init(s);memset(l,0,sizeof(*l));l->player=s->player;
}
static int near(float x,float y){return fabsf(x-y)<0.002f;}
static void populate(RikituScene *s,RikituLoop *l){
  init(s,l);
  s->active_checkpoint=4490.0f;s->reached_checkpoints=2;
  s->hearts=2;s->damage_count=5;
  s->stage.fragments[0]=1;s->stage.fragments[2]=1;
  s->stage.fragment_count=2;
  s->stage.coins[0]=1;s->stage.coins[23]=1;s->stage.coins[89]=1;
  s->stage.coin_count=3;
  s->enemies[0].dead=1;s->enemies[5].dead=1;
  l->player.x=4700.0f;l->player.y=420.0f;
  l->active_seconds=123.456f;
}
int main(void){
  RikituScene s,d;
  RikituLoop l,r;
  uint8_t bytes[RIKITU_SAVE_BYTES],corrupt[RIKITU_SAVE_BYTES];
  uint32_t generation=0;
  FILE *f;int i;
  (void)remove(FILE_A);(void)remove(FILE_B);
  (void)remove(WRONG_A);(void)remove(WRONG_B);
  init(&s,&l);
  assert(rikitu_save_encode(bytes,&s,&l,1));
  assert(bytes[0]=='R'&&bytes[1]=='K'&&bytes[2]=='S'&&bytes[3]=='1');
  assert(rikitu_save_decode(&d,&r,bytes,sizeof(bytes),&generation));
  assert(generation==1 && near(d.active_checkpoint,105));
  populate(&s,&l);
  /* A map RKS1 writer must never overwrite a campaign RKC2 path. */
  write_bytes(WRONG_A,"keep-a",6);write_bytes(WRONG_B,"keep-b",6);
  assert(!rikitu_save_store_pair(WRONG_A,WRONG_B,&s,&l));
  assert(!rikitu_save_load_pair(WRONG_A,WRONG_B,&d,&r));
  assert_bytes(WRONG_A,"keep-a",6);assert_bytes(WRONG_B,"keep-b",6);
  assert(rikitu_save_encode(bytes,&s,&l,12));
  assert(rikitu_save_decode(&d,&r,bytes,sizeof(bytes),&generation));
  assert(generation==12 && d.level_id==1 && d.hearts==2);
  assert(d.reached_checkpoints==2 && near(d.active_checkpoint,4490));
  assert(d.stage.fragment_count==2&&d.stage.coin_count==3);
  assert(d.stage.fragments[0]&&d.stage.fragments[2]&&!d.stage.fragments[1]);
  assert(d.stage.coins[0]&&d.stage.coins[23]&&d.stage.coins[89]);
  assert(d.enemies[0].dead&&d.enemies[5].dead&&!d.enemies[1].dead);
  assert(near(r.player.x,4490)&&near(r.player.y,413));
  assert(near(d.invulnerability_seconds,1.0f)&&near(r.active_seconds,123.456f));
  assert(!r.player.grounded && near(r.player.vy,0));
  memcpy(corrupt,bytes,sizeof(bytes));
  corrupt[17]^=0x20;
  init(&d,&r);
  d.hearts=3;
  assert(!rikitu_save_decode(&d,&r,corrupt,sizeof(corrupt),NULL));
  assert(d.hearts==3&&near(d.active_checkpoint,105));
  assert(!rikitu_save_decode(&d,&r,bytes,sizeof(bytes)-1,NULL));
  assert(!rikitu_save_decode(&d,&r,bytes,sizeof(bytes)+1,NULL));
  assert(!rikitu_save_decode(NULL,&r,bytes,sizeof(bytes),NULL));
  assert(!rikitu_save_encode(bytes,&s,&l,0));
  s.reached_checkpoints=1;
  assert(!rikitu_save_encode(bytes,&s,&l,5));
  s.reached_checkpoints=2;
  s.stage.coin_count=4;
  assert(!rikitu_save_encode(bytes,&s,&l,5));
  s.stage.coin_count=3;
  s.hearts=0;
  assert(!rikitu_save_encode(bytes,&s,&l,5));
  s.hearts=2;
  assert(!rikitu_save_store_pair(FILE_A,FILE_A,&s,&l));
  assert(!rikitu_save_load_pair(FILE_A,FILE_B,&d,&r));
  /* Three successive saves alternate file slots; older stays recoverable. */
  assert(rikitu_save_store_pair(FILE_A,FILE_B,&s,&l));
  /* A load API must not treat one file as a two-slot recovery pair. */
  init(&d,&r);
  d.hearts=3;
  r.active_seconds=777.0f;
  assert(!rikitu_save_load_pair(FILE_A,FILE_A,&d,&r));
  assert(d.hearts==3 && near(r.active_seconds,777.0f));
  s.hearts=1;
  assert(rikitu_save_store_pair(FILE_A,FILE_B,&s,&l));
  init(&d,&r);
  assert(rikitu_save_load_pair(FILE_A,FILE_B,&d,&r)&&d.hearts==1);
  s.hearts=2;
  assert(rikitu_save_store_pair(FILE_A,FILE_B,&s,&l));
  assert(rikitu_save_load_pair(FILE_A,FILE_B,&d,&r)&&d.hearts==2);
  /* Slot A now contains newest gen3; corrupt it to verify fallback B(gen2). */
  f=fopen(FILE_A,"r+b");assert(f);
  assert(fseek(f,19,SEEK_SET)==0);
  assert(fputc(255,f)!=EOF);assert(fclose(f)==0);
  assert(rikitu_save_load_pair(FILE_A,FILE_B,&d,&r)&&d.hearts==1);
  /* A partial new slot never destroys the surviving B checkpoint. */
  f=fopen(FILE_A,"wb");assert(f);
  for(i=0;i<8;i++)assert(fputc('X',f)!=EOF);
  assert(fclose(f)==0);
  assert(rikitu_save_load_pair(FILE_A,FILE_B,&d,&r)&&d.hearts==1);
  (void)remove(FILE_A);(void)remove(FILE_B);
  (void)remove(WRONG_A);(void)remove(WRONG_B);
  assert(!rikitu_save_load_pair(FILE_A,FILE_B,&d,&r));
  puts("PASS MAP01 dual-slot save v1: CRC, 90 coins, six enemies, checkpoint, hits, time, corrupt/partial fallback");
  return 0;
}
