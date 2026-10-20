/* Compare native C99 outputs directly against the browser's JS game-core. */
'use strict';
const assert=require('node:assert/strict');
const {execFileSync}=require('node:child_process');
const core=require('../game-core.js');
const binary=process.argv[2];
if(!binary)throw Error('usage: node tests/test-native-parity.cjs <probe-executable>');
const text=execFileSync(binary,{encoding:'utf8',maxBuffer:2*1024*1024});
const reasons={'ready':0,'insufficient-coins':1,'not-unlocked':2,
               'not-in-shop-or-maxed':3};
let count=0;
for(const line of text.trim().split(/\r?\n/)){
 const [tag,...n]=line.trim().split(/\s+/);
 const v=n.map(Number);
 assert(v.every(Number.isFinite),line);
 if(tag==='box'){
   assert.deepEqual(v,[
     Number(core.boxHit({x:0,y:0,w:10,h:10},{x:9,y:9,w:1,h:1})),
     Number(core.boxHit({x:0,y:0,w:10,h:10},{x:10,y:0,w:2,h:2}))
   ]);
 } else if(tag==='gate'){
   assert.deepEqual(v,[Number(core.canExit(2,false,true)),
     Number(core.canExit(3,true,false)),Number(core.canExit(3,true,true))]);
 } else if(tag==='jump'){
   assert.deepEqual(v,[Number(core.playerCanJump({grounded:false,coyote:0})),
     Number(core.playerCanJump({grounded:false,coyote:.01})),
     Number(core.playerCanJump({grounded:true,coyote:0}))]);
 } else if(tag==='stomp'){
   assert.deepEqual(v,[Number(core.stompFromAbove(95,102,100,true)),
     Number(core.stompFromAbove(113,120,100,true)),
     Number(core.stompFromAbove(95,102,100,false))]);
 } else if(tag==='unlock'){
   const [stage,shop,unlocks]=v;
   assert.equal(shop,Number(core.isShopStage(stage)),line);
   assert.equal(unlocks,core.unlockedUpgrades(stage),line);
 } else if(tag==='offer'){
   const [stage,bought,coins,available,reason,price,hearts,unlocked,
     success,afterCount,afterCoins,afterHearts]=v;
   const o=core.nextHeartPurchase(stage,bought,coins);
   const purchase=core.buyHeart(stage,bought,coins);
   assert.deepEqual([available,reason,price,hearts,unlocked],
     [Number(o.available),reasons[o.reason],o.price??0,o.heartsAfter??0,o.unlocked??0],
     line);
   assert.deepEqual([success,afterCount,afterCoins,afterHearts],
     [Number(purchase.success),purchase.purchasedCount,purchase.coins,purchase.hearts],
     line);
 } else throw Error('unexpected probe line: '+line);
 count++;
}
assert(count>1000,'insufficient parity cases');
console.log('PASS JS vs native C99 parity: '+count+' independent rule comparisons');
