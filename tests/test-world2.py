"""Playable World II browser test: conveyor, steam lift, boss, Moki purchases and save."""
from pathlib import Path
import json, os
from playwright.sync_api import sync_playwright
ROOT=Path(__file__).resolve().parents[1]
HTML=(ROOT/'RIKITU_play_now.html').read_text(encoding='utf-8')
KEY='rikitu-zavodnye-tropy-checkpoint-v1'
SHIM="""(data)=>{const box=Object.assign({},data);
Object.defineProperty(window,'localStorage',{configurable:true,value:{
getItem(k){return Object.prototype.hasOwnProperty.call(box,k)?box[k]:null},
setItem(k,v){box[k]=String(v)},removeItem(k){delete box[k]}}});}"""
def state(page):return page.evaluate('RIKITU_TEST_STATE()')
def move(page,x,y):page.evaluate('(p)=>RIKITU_TEST_MOVE(...p)',[x,y])
def parts(page,coords):
    for n,(x,y) in enumerate(coords,1):
        move(page,x,y)
        page.wait_for_function('(n)=>RIKITU_TEST_STATE().fragments>=n',arg=n,timeout=5000)
    assert state(page)['fragments']==3,state(page)
def boss(page):
    for hp in (2,1,0):
        page.wait_for_timeout(1000)
        for tries in range(4):
            s=state(page);move(page,s['bossX']+14,s['bossY']-58)
            try:
                page.wait_for_function('(hp)=>RIKITU_TEST_STATE().bossHP===hp',arg=hp,timeout=1100)
                break
            except Exception:
                if tries==3:raise AssertionError(f'Boss did not receive stomp: {state(page)}')
with sync_playwright() as pw:
    binary=os.environ.get('RIKITU_CHROMIUM_PATH') or ('/usr/bin/chromium' if Path('/usr/bin/chromium').is_file() else None)
    browser=pw.chromium.launch(headless=True,executable_path=binary,args=['--no-sandbox'])
    page=browser.new_page(viewport={'width':1200,'height':760})
    errors=[];page.on('pageerror',lambda err:errors.append(str(err)))
    # A valid save at the exit of World I, before playing stage 5.
    save={'version':1,'levelIndex':3,'mode':'between','wallet':360,'purchased':0,
          'stageCoins':0,'fragments':[],'coins':[],'switches':[],
          'checkpoint':105,'hearts':3,'bossDown':True}
    page.evaluate(SHIM,{KEY:json.dumps(save,ensure_ascii=False)})
    page.set_content(HTML,wait_until='load')
    page.keyboard.press('Enter')
    page.wait_for_function("()=>RIKITU_TEST_STATE().mode==='between'",timeout=5000)
    page.keyboard.press('Enter')
    page.wait_for_function("()=>RIKITU_TEST_STATE().level===5",timeout=5000)
    levels=page.evaluate('RIKITU_TEST_LEVELS()')
    assert len(levels)==6 and all(x['coins']==90 and len(x['fragments'])==3 for x in levels)
    assert len(levels[4]['conveyors'])==3 and len(levels[5]['vents'])==3
    move(page,1190,413);page.wait_for_timeout(280)
    assert state(page)['playerX']>1190,state(page)
    assert page.evaluate('RIKITU_TEST_COLLECT_COINS(90)')
    parts(page,[(1690,351),(4580,352),(7770,351)])
    move(page,8970,400)
    page.wait_for_function("()=>RIKITU_TEST_STATE().mode==='between'",timeout=5000)
    assert state(page)['wallet']==450,state(page)
    page.keyboard.press('Enter')
    page.wait_for_function("()=>RIKITU_TEST_STATE().level===6",timeout=5000)
    move(page,1350,413);page.wait_for_timeout(260)
    assert state(page)['playerY']<413,state(page)
    assert page.evaluate('RIKITU_TEST_COLLECT_COINS(90)')
    parts(page,[(1800,348),(5210,346),(8110,351)])
    move(page,9750,400);page.wait_for_timeout(150)
    assert state(page)['mode']=='playing',state(page)
    boss(page)
    move(page,9750,400)
    page.wait_for_function("()=>RIKITU_TEST_STATE().mode==='shop'",timeout=5000)
    assert state(page)['wallet']==540,state(page)
    page.keyboard.press('b')
    page.wait_for_function("()=>RIKITU_TEST_STATE().bought===1",timeout=5000)
    assert state(page)['wallet']==420 and state(page)['maxHearts']==4,state(page)
    page.keyboard.press('b')
    page.wait_for_function("()=>RIKITU_TEST_STATE().bought===2",timeout=5000)
    assert state(page)['wallet']==260 and state(page)['maxHearts']==5,state(page)
    page.keyboard.press('b');page.wait_for_timeout(160)
    assert state(page)['wallet']==260 and state(page)['bought']==2,state(page)
    saved=page.evaluate('(key)=>localStorage.getItem(key)',KEY)
    assert saved and '"purchased":2' in saved,saved
    # A player who completed the old four-level demo keeps their progress.
    page.close()
    old={'version':1,'levelIndex':3,'mode':'victory','wallet':200,'purchased':1,
         'stageCoins':0,'fragments':[0,1,2],'coins':[],'switches':[],
         'checkpoint':6900,'hearts':4,'bossDown':True}
    page=browser.new_page(viewport={'width':1200,'height':760})
    page.on('pageerror',lambda err:errors.append(str(err)))
    page.evaluate(SHIM,{KEY:json.dumps(old,ensure_ascii=False)})
    page.set_content(HTML,wait_until='load')
    page.keyboard.press('Enter')
    page.wait_for_function("()=>RIKITU_TEST_STATE().mode==='between'",timeout=5000)
    page.keyboard.press('Enter')
    page.wait_for_function("()=>RIKITU_TEST_STATE().level===5",timeout=5000)
    assert state(page)['wallet']==200 and state(page)['maxHearts']==4,state(page)
    assert not errors,errors
    print('PASS World II: stages 5-6, boss, two Moki purchases, migration of old victory save')
    browser.close()
