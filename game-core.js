/* RIKITU: Zavodnye Tropy — portable rules, no copyrighted visual assets.
 * This module works in both browsers (window.RikituCore) and Node (require).
 */
(function(root, make) {
  const api = make();
  if (typeof module !== 'undefined' && module.exports) module.exports = api;
  root.RikituCore = api;
})(typeof globalThis !== 'undefined' ? globalThis : this, function(){
  'use strict';
  const SETTINGS = Object.freeze({
    gravity: 1580,
    jumpSpeed: 625,
    walkSpeed: 225,
    runSpeed: 265,
    playerWidth: 33,
    playerHeight: 43,
    startingHearts: 3,
    maxHearts: 7,
    coinsPerStage: 90,
    heartUnlockStages: [2,6,10,14],
    heartUpgradePrices: [120,160,200,240],
    fragmentsPerStage: 3,
    checkpointX: [1260, 1430],
    invincibility: 1.6,
    coyoteTime: 0.115,
    jumpBuffer: 0.12,
  });
  function boxHit(a,b){return a.x < b.x+b.w && a.x+a.w > b.x && a.y < b.y+b.h && a.y+a.h>b.y;}
  function canExit(fragments,bossRequired,bossDefeated){
    return fragments>=SETTINGS.fragmentsPerStage && (!bossRequired || bossDefeated);
  }
  function collectFragment(collected,id) {const after = new Set(collected);after.add(id);return after;}
  function playerCanJump(p) {return !!p.grounded || p.coyote>0;}
  function stompFromAbove(prevBottom, nowBottom, targetTop, falling){
    return falling && prevBottom<=targetTop+12 && nowBottom >=targetTop;
  }
  function heartAfterDamage(hearts) {return Math.max(0,hearts-1);}
  function isShopStage(stage){return Number.isInteger(stage)&&stage>=2&&stage<=16&&stage%2===0;}
  function unlockedUpgrades(clearedStage){
    return SETTINGS.heartUnlockStages.filter(s=>clearedStage>=s).length;
  }
  function nextHeartPurchase(clearedStage,purchasedCount,coins){
    const limit=unlockedUpgrades(clearedStage);
    if(!isShopStage(clearedStage)||!Number.isInteger(purchasedCount)||purchasedCount<0||purchasedCount>=4)
      return {available:false,reason:'not-in-shop-or-maxed'};
    if(purchasedCount>=limit)return {available:false,reason:'not-unlocked'};
    const price=SETTINGS.heartUpgradePrices[purchasedCount];
    return {available:coins>=price,reason:coins>=price?'ready':'insufficient-coins',price,
      heartsAfter:SETTINGS.startingHearts+purchasedCount+1,unlocked:limit};
  }
  function buyHeart(clearedStage,purchasedCount,coins){
    const offer=nextHeartPurchase(clearedStage,purchasedCount,coins);
    if(!offer.available)return {success:false,purchasedCount,coins,hearts:SETTINGS.startingHearts+purchasedCount};
    return {success:true,purchasedCount:purchasedCount+1,coins:coins-offer.price,hearts:offer.heartsAfter};
  }
  return {SETTINGS,boxHit,canExit,collectFragment,playerCanJump,stompFromAbove,heartAfterDamage,
    isShopStage,unlockedUpgrades,nextHeartPurchase,buyHeart};
});
