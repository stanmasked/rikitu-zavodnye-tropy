/* Prevent regressions in checkpoint saving across MAP02–MAP16.
 * Source-level audit; actual SD durability needs runtime/hardware tests.
 */
'use strict';
const assert=require('node:assert/strict');
const fs=require('node:fs');
const path=require('node:path');
const main=fs.readFileSync(path.join(__dirname,'../switch_graphical/source/main.c'),'utf8');
const make=fs.readFileSync(path.join(__dirname,'../switch_graphical/Makefile'),'utf8');
assert(main.includes('#define LEVEL_A "sdmc:/switch/RIKITU/level-a.rkl"'));
assert(main.includes('#define LEVEL_B "sdmc:/switch/RIKITU/level-b.rkl"'));
assert(main.includes('rikitu_level_save_load_pair(LEVEL_A,LEVEL_B,'));
assert(main.includes('rikitu_level_save_store_pair(LEVEL_A,LEVEL_B,'));
assert(main.includes('campaign.level>1 && !campaign_debug && save_ready'));
assert(main.includes('!scene.stage.completed &&'));
assert(main.includes('level_save_elapsed>=2.0f'));
assert(main.includes('memcmp(level_now,level_last,28)!=0'));
assert(main.includes('level_now[40]!=level_last[40]'));
assert(main.includes('level_have_last=0'));
assert(main.includes('campaign.mode==RIKITU_CAMPAIGN_PLAYING &&'));
assert(main.includes('campaign.level>1 && !scene.stage.completed'));
assert(make.includes('rikitu_level_save.c rikitu_level_save_io.c'));
/* No RKL3 file use inside the Y and X debug sections. */
const a=main.indexOf('if(pressed & HidNpadButton_Y){');
const b=main.indexOf('}else if(campaign.mode==RIKITU_CAMPAIGN_SHOP){',a);
assert(a>=0&&b>a);
const debug=main.slice(a,b);
assert(!debug.includes('rikitu_level_save_load_pair'));
assert(!debug.includes('rikitu_level_save_store_pair'));
assert(!debug.includes('remove(LEVEL_A)'));
assert(!debug.includes('remove(LEVEL_B)'));
assert(main.includes('map01-a.rks')&&main.includes('campaign-a.rkc'));
console.log('PASS SD RKL3 MAP02–16 restore and throttled isolated checkpoint writes');
