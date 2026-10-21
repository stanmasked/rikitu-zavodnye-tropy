/* Regression: diagnostic Y/X can never touch persisted RKS1/RKC2 slots.
 * This is static source guard coverage, not a Nintendo hardware save test.
 */
'use strict';
const assert=require('node:assert/strict');
const fs=require('node:fs');
const path=require('node:path');
const src=fs.readFileSync(path.join(__dirname,'../switch_graphical/source/main.c'),'utf8');
const slice=(from,to)=>{
 const a=src.indexOf(from),b=src.indexOf(to,a);
 assert(a>=0&&b>a,'missing control flow section: '+from);
 return src.slice(a,b);
};
const debug=slice('if(pressed & HidNpadButton_Y){','}else if(campaign.mode==RIKITU_CAMPAIGN_SHOP){');
assert(debug.includes('campaign_debug=1;'));
assert(debug.includes('rikitu_campaign_init(&campaign,selected)'));
for(const forbidden of ['rikitu_save_store_pair','rikitu_save_load_pair',
  'rikitu_campaign_save_store_pair','rikitu_campaign_save_load_pair',
  'remove(MAP01_A)','remove(MAP01_B)','remove(CAMPAIGN_A)','remove(CAMPAIGN_B)']){
 assert(!debug.includes(forbidden),'debug session accessed permanent data: '+forbidden);
}
const plus=slice('if(pressed & HidNpadButton_Plus){','/* Y/X start a disposable');
assert(plus.includes('if(save_ready && !campaign_debug)'));
assert(plus.includes('if(save_ready && !campaign_debug &&'));
const game=slice('if(campaign.mode==RIKITU_CAMPAIGN_PLAYING){','frame=(uint32_t*)framebufferBegin');
assert(game.includes('if(save_ready && !campaign_debug)'));
assert(game.includes('if(scene.level_id==1 && save_ready && !campaign_debug)'));
const auto=slice('framebufferEnd(&fb);','/* A short yielding sleep');
assert(auto.includes('!campaign_debug && rikitu_save_encode(now,&scene,&game,1)'));
const campaignSaves=[...src.matchAll(/rikitu_campaign_save_store_pair\(CAMPAIGN_A,CAMPAIGN_B,&campaign\)/g)].length;
assert(campaignSaves>=4,'campaign saves no longer wired');
assert(src.includes('#define MAP01_A "sdmc:/switch/RIKITU/map01-a.rks"'));
assert(src.includes('#define CAMPAIGN_A "sdmc:/switch/RIKITU/campaign-a.rkc"'));
console.log('PASS: Y/X isolated from permanent MAP01 RKS1 and campaign RKC2 files');
