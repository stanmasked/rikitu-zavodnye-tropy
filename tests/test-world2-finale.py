"""World II finale: stages 7-8, boss, clock spring, Moki and saved upgrades."""
from pathlib import Path
import json, os
from playwright.sync_api import sync_playwright
ROOT=Path(__file__).resolve().parents[1]
HTML=(ROOT/'RIKITU_play_now.html').read_text(encoding='utf-8')
KEY='rikitu-zavodnye-tropy-checkpoint-v1'
SHIM="""(initial)=>{
 const data=Object.assign({},initial);
 Object.defineProperty(window,'localStorage',{configurable:true,value:{
   getItem(k){return Object.hasOwn(data,k)?data[k]:null},
   setItem(k,v){data[k]=String(v)},removeItem(k){delete data[k]}
 }});
}"""
def state(page):return page.evaluate('RIKITU_TEST_STATE()')
def move(page,x,y):page.evaluate('(p)=>RIKITU_TEST_MOVE(...p)',[x,y])
def collect(page,coords):
    for count,(x,y) in enumerate(coords,1):
        move(page,x,y)
        page.wait_for_function('(n)=>RIKITU_TEST_STATE().fragments>=n',arg=count,timeout=5000)
def spawn(browser,save):
    page=browser.new_page(viewport={'width':1200,'height':760})
    page.evaluate(SHIM,{KEY:json.dumps(save,ensure_ascii=False)})
    page.set_content(HTML,wait_until='load')
    return page
def clear_city(page):
    page.keyboard.press('Enter')
    page.wait_for_function("()=>RIKITU_TEST_STATE().mode==='between'",timeout=5000)
    page.keyboard.press('Enter')
    page.wait_for_function("()=>RIKITU_TEST_STATE().level===7",timeout=5000)
    levels=page.evaluate('RIKITU_TEST_LEVELS()')
    assert len(levels)==8 and all(d['coins']==90 and len(d['fragments'])==3 for d in levels)
    assert len(levels[6]['clockPads'])==3 and len(levels[7]['switches'])==3
    move(page,1355,413)
    page.wait_for_function('()=>RIKITU_TEST_STATE().playerY<400',timeout=5000)
    collect(page,[(1730,351),(4750,351),(8100,349)])
    move(page,9310,400)
    page.wait_for_function("()=>RIKITU_TEST_STATE().mode==='between'",timeout=5000)
    page.keyboard.press('Enter')
    page.wait_for_function("()=>RIKITU_TEST_STATE().level===8",timeout=5000)
    collect(page,[(1810,349),(5070,349),(8440,350)])
    move(page,9998,400)
    page.wait_for_timeout(170)
    assert state(page)['mode']=='playing',state(page)
    for hp in (2,1,0):
        page.wait_for_timeout(1000)
        for attempt in range(4):
            boss=state(page)
            move(page,boss['bossX']+14,boss['bossY']-58)
            try:
                page.wait_for_function('(hp)=>RIKITU_TEST_STATE().bossHP===hp',arg=hp,timeout=1400)
                break
            except Exception:
                if attempt==3:raise AssertionError(f'Boss stomp failed: {state(page)}')
    move(page,9998,400)
    page.wait_for_function("()=>RIKITU_TEST_STATE().mode==='shop'",timeout=5000)
with sync_playwright() as pw:
    binary=os.environ.get('RIKITU_CHROMIUM_PATH') or ('/usr/bin/chromium' if Path('/usr/bin/chromium').is_file() else None)
    browser=pw.chromium.launch(headless=True,executable_path=binary,args=['--no-sandbox'])
    errors=[]
    # Stage-six victory from older demo -> seventh level; third heart remains locked.
    old={'version':1,'levelIndex':5,'mode':'victory','wallet':260,'purchased':2,
         'stageCoins':90,'fragments':[0,1,2],'coins':list(range(90)),
         'switches':[],'checkpoint':7360,'hearts':5,'bossDown':True}
    page=spawn(browser,old)
    page.on('pageerror',lambda e:errors.append(str(e)))
    clear_city(page)
    before=state(page)
    assert before['bought']==2 and before['maxHearts']==5,before
    page.keyboard.press('b')
    page.wait_for_timeout(120)
    assert state(page)['wallet']==before['wallet'] and state(page)['bought']==2
    page.keyboard.press('Enter')
    page.wait_for_function("()=>RIKITU_TEST_STATE().mode==='victory'",timeout=5000)
    page.close()
    # Player delayed both prior hearts; can buy them now, but not a third.
    skipped={'version':1,'levelIndex':5,'mode':'victory','wallet':280,'purchased':0,
         'stageCoins':90,'fragments':[0,1,2],'coins':list(range(90)),
         'switches':[],'checkpoint':7360,'hearts':3,'bossDown':True}
    page=spawn(browser,skipped)
    page.on('pageerror',lambda e:errors.append(str(e)))
    clear_city(page)
    balance=state(page)['wallet']
    page.keyboard.press('b')
    page.wait_for_function('()=>RIKITU_TEST_STATE().bought===1',timeout=5000)
    page.keyboard.press('b')
    page.wait_for_function('()=>RIKITU_TEST_STATE().bought===2',timeout=5000)
    assert state(page)['maxHearts']==5 and state(page)['wallet']==balance-280,state(page)
    saved=page.evaluate('(k)=>JSON.parse(localStorage.getItem(k))',KEY)
    assert saved['purchased']==2 and saved['mode']=='shop',saved
    assert not errors,errors
    print('PASS city finale: stages 7-8, clock spring, boss, Moki limit, save carryover')
    browser.close()
