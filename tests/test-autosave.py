"""Autonomous save/restore browser test with a deterministic storage shim.
Chromium file:// access is not required; storage methods behave like localStorage.
"""
from pathlib import Path
import json
import os
from playwright.sync_api import sync_playwright

ROOT=Path(__file__).resolve().parents[1]
HTML=(ROOT/'RIKITU_play_now.html').read_text(encoding='utf-8')
KEY='rikitu-zavodnye-tropy-checkpoint-v1'
SHIM="""(initial)=>{
 const data=Object.assign({},initial);window.fakeStorage=data;
 Object.defineProperty(window,'localStorage',{configurable:true,value:{
   getItem(k){return Object.hasOwn(data,k)?data[k]:null},
   setItem(k,v){data[k]=String(v)},removeItem(k){delete data[k]}
 }});
}"""

def make_page(browser,saved=None):
    page=browser.new_page(viewport={'width':1200,'height':760})
    page.evaluate(SHIM,{KEY:saved} if saved else {})
    page.set_content(HTML,wait_until='load')
    return page

with sync_playwright() as p:
    binary=os.environ.get('RIKITU_CHROMIUM_PATH') or ('/usr/bin/chromium' if Path('/usr/bin/chromium').is_file() else None)
    browser=p.chromium.launch(headless=True,executable_path=binary,args=['--no-sandbox'])
    page=make_page(browser)
    errors=[]
    page.on('pageerror',lambda e:errors.append(str(e)))
    page.keyboard.press('Enter');page.wait_for_timeout(130)
    assert page.evaluate('RIKITU_TEST_STATE().mode')=='playing'
    page.wait_for_timeout(230)
    elapsed=page.evaluate('RIKITU_TEST_STATE().elapsedSeconds')
    assert elapsed>0.10,elapsed
    page.keyboard.press('p')
    page.wait_for_timeout(80)
    assert page.evaluate('RIKITU_TEST_STATE().mode')=='paused'
    paused=page.evaluate('RIKITU_TEST_STATE().elapsedSeconds')
    page.wait_for_timeout(220)
    assert abs(page.evaluate('RIKITU_TEST_STATE().elapsedSeconds')-paused)<0.03
    page.keyboard.press('p')
    page.wait_for_timeout(90)
    assert page.evaluate('RIKITU_TEST_STATE().mode')=='playing'
    page.evaluate('RIKITU_TEST_MOVE(1520,354)');page.wait_for_timeout(110)
    assert page.evaluate('RIKITU_TEST_STATE().fragments')==1
    page.evaluate('RIKITU_TEST_MOVE(2100,410)');page.wait_for_timeout(170)
    state=page.evaluate('RIKITU_TEST_STATE()')
    assert state['checkpoint']==2050,state
    saved=page.evaluate('(key)=>localStorage.getItem(key)',KEY)
    assert saved and '"fragments":[0]' in saved,saved
    saved_json=json.loads(saved)
    assert saved_json['seconds']>0 and saved_json['hits']==0,saved_json
    page.close()
    
    page=make_page(browser,saved)
    page.on('pageerror',lambda e:errors.append(str(e)))
    assert page.evaluate('RIKITU_TEST_STATE().mode')=='title'
    page.keyboard.press('Enter');page.wait_for_timeout(140)
    restored=page.evaluate('RIKITU_TEST_STATE()')
    assert restored['checkpoint']==2050 and restored['fragments']==1 and restored['playerX']==2050,restored
    assert restored['elapsedSeconds']>=saved_json['seconds']-0.02,restored
    assert restored['hitCount']==0,restored
    page.keyboard.press('r');page.wait_for_timeout(130)
    reset=page.evaluate('RIKITU_TEST_STATE()')
    assert reset['fragments']==0 and reset['checkpoint']==105,reset
    # A legal Moki purchase can leave wallet (60) below coins collected this level (90).
    # Such a save MUST be accepted and must not offer the same heart a second time.
    page.close()
    purchased_save={
        'version':1,'levelIndex':1,'mode':'shop','wallet':60,
        'purchased':1,'stageCoins':90,'fragments':[0,1,2],
        'coins':list(range(90)),'switches':[],
        'checkpoint':6980,'hearts':4,'bossDown':True
    }
    page=make_page(browser,json.dumps(purchased_save,ensure_ascii=False))
    page.on('pageerror',lambda e:errors.append(str(e)))
    assert page.evaluate('RIKITU_TEST_STATE().mode')=='title'
    page.keyboard.press('Enter');page.wait_for_timeout(130)
    shop=page.evaluate('RIKITU_TEST_STATE()')
    assert shop['mode']=='shop' and shop['maxHearts']==4 and shop['wallet']==60 and shop['bought']==1,shop
    assert shop['elapsedSeconds']==0 and shop['hitCount']==0,shop
    page.keyboard.press('b');page.wait_for_timeout(110)
    shop_again=page.evaluate('RIKITU_TEST_STATE()')
    assert shop_again['wallet']==60 and shop_again['bought']==1,shop_again
    assert not errors,errors
    print('PASS browser autosave: checkpoint, fragments, reset, purchased Moki heart')
    browser.close()
