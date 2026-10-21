/* Stage95 host process-isolation checkpoint test.
 * Run as three separate OS processes, sharing only actual RKS1 files:
 *   binary seed <slot-A> <slot-B>
 *   binary resume <slot-A> <slot-B>
 *   binary verify <slot-A> <slot-B>
 * Validates RIKITU's existing save/restore logic independently of NeXium
 * guest FS IPC. No direct scene editing occurs AFTER writing each save.
 */
#include "rikitu_save.h"
#include "rikitu_flower.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

static int near(float a,float b){return fabsf(a-b)<0.02f;}
static void fresh(RikituScene *s,RikituLoop *l){
  rikitu_flower_init(s);
  memset(l,0,sizeof(*l));
  l->player=s->player;
}
static void inspect_slot(const char *path,uint32_t generation,
                         float checkpoint){
  uint8_t data[RIKITU_SAVE_BYTES];
  RikituScene s;RikituLoop l;
  uint32_t gen=0;
  FILE *f=fopen(path,"rb");
  assert(f);
  assert(fread(data,1,sizeof(data),f)==sizeof(data));
  assert(fgetc(f)==EOF);
  assert(fclose(f)==0);
  assert(data[0]=='R'&&data[1]=='K'&&data[2]=='S'&&data[3]=='1');
  fresh(&s,&l);
  assert(rikitu_save_decode(&s,&l,data,sizeof(data),&gen));
  assert(gen==generation);
  assert(near(s.active_checkpoint,checkpoint));
}
int main(int argc,char **argv){
  RikituScene s;RikituLoop loop;
  const char *phase,*a,*b;
  if(argc!=4){
    fprintf(stderr,"usage: %s seed|resume|verify slotA slotB\n",argv[0]);
    return 2;
  }
  phase=argv[1];a=argv[2];b=argv[3];
  assert(strcmp(a,b)!=0);
  fresh(&s,&loop);
  if(strcmp(phase,"seed")==0){
    /* First run: never read stale data and create exactly slot A gen1. */
    (void)remove(a);(void)remove(b);
    assert(!rikitu_save_load_pair(a,b,&s,&loop));
    s.active_checkpoint=2050.0f;
    s.reached_checkpoints=1;
    s.hearts=2;
    s.stage.fragments[0]=1;s.stage.fragment_count=1;
    s.stage.coins[0]=1;s.stage.coin_count=1;
    s.enemies[0].dead=1;
    loop.active_seconds=25.0f;
    assert(rikitu_save_store_pair(a,b,&s,&loop));
    inspect_slot(a,1u,2050.0f);
    assert(!fopen(b,"rb"));
    puts("PASS cold process 1: seed RKS1 slot A generation 1");
  }else if(strcmp(phase,"resume")==0){
    /* A new process must recover all previously committed data. */
    assert(rikitu_save_load_pair(a,b,&s,&loop));
    assert(near(s.active_checkpoint,2050.0f) &&
           near(loop.player.x,2050.0f));
    assert(s.hearts==2&&s.stage.fragment_count==1&&
           s.stage.coin_count==1&&s.enemies[0].dead);
    assert(near(loop.active_seconds,25.0f));
    s.active_checkpoint=4490.0f;
    s.reached_checkpoints=2;
    s.hearts=1;
    s.stage.fragments[2]=1;s.stage.fragment_count=2;
    s.stage.coins[89]=1;s.stage.coin_count=2;
    s.enemies[5].dead=1;
    loop.active_seconds=55.0f;
    assert(rikitu_save_store_pair(a,b,&s,&loop));
    inspect_slot(a,1u,2050.0f);
    inspect_slot(b,2u,4490.0f);
    puts("PASS cold process 2: restore generation 1 and commit slot B generation 2");
  }else if(strcmp(phase,"verify")==0){
    /* Third process reads newest slot; save remains untouched. */
    assert(rikitu_save_load_pair(a,b,&s,&loop));
    assert(near(s.active_checkpoint,4490.0f) &&
           near(loop.player.x,4490.0f));
    assert(s.reached_checkpoints==2 && s.hearts==1 &&
           s.stage.fragment_count==2 && s.stage.coin_count==2);
    assert(s.stage.fragments[0]&&s.stage.fragments[2] &&
           s.stage.coins[0]&&s.stage.coins[89]);
    assert(s.enemies[0].dead&&s.enemies[5].dead);
    assert(near(loop.active_seconds,55.0f));
    inspect_slot(a,1u,2050.0f);
    inspect_slot(b,2u,4490.0f);
    puts("PASS cold process 3: load generation 2 without writing");
  }else{
    fprintf(stderr,"unknown stage95 test phase: %s\n",phase);
    return 2;
  }
  return 0;
}
