"""World III finale: Crystal Caves, Silent Mine, sixth boss, Moki shop limit."""
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
with sync_playwright() as pw:
    exe=os.environ.get('RIKITU_CHROMIUM_PATH') or ('/usr/bin/chromium' if Path('/usr/bin/chromium').is_file() else None)
    browser=pw.chromium.launch(headless=True,executable_path=exe,args=['--no-sandbox'])
    page=browser.new_page(viewport={'width':1200,'height':760})
    errors=[]
    page.on('pageerror',lambda e:errors.append(str(e)))
    save={'version':1,'levelIndex':9,'mode':'victory','wallet':420,'purchased':3,
          'stageCoins':90,'fragments':[0,1,2],'coins':list(range(90)),
          'switches':[],'checkpoint':8020,'hearts':6,'bossDown':True}
    page.evaluate(SHIM,{KEY:json.dumps(save,ensure_ascii=False)})
    page.set_content(HTML,wait_until='load')
    levels=page.evaluate('RIKITU_TEST_LEVELS()')
    assert len(levels)==14 and all(x['coins']==90 and len(x['fragments'])==3 for x in levels)
    assert len(levels[10]['crystals'])==3 and len(levels[11]['vents'])==3
    page.keyboard.press('Enter')
    page.wait_for_function("()=>RIKITU_TEST_STATE().mode==='between'",timeout=5000)
    page.keyboard.press('Enter')
    page.wait_for_function('()=>RIKITU_TEST_STATE().level===11',timeout=5000)
    assert state(page)['wallet']==420 and state(page)['maxHearts']==6
    move(page,1135,413)
    page.wait_for_function('()=>RIKITU_TEST_STATE().playerY<391',timeout=5000)
    collect(page,[(1760,349),(4920,347),(8430,350)])
    move(page,9640,400)
    page.wait_for_function("()=>RIKITU_TEST_STATE().mode==='between'",timeout=5000)
    page.keyboard.press('Enter')
    page.wait_for_function('()=>RIKITU_TEST_STATE().level===12',timeout=5000)
    collect(page,[(1800,349),(5590,348),(9120,351)])
    move(page,10544,400)
    page.wait_for_timeout(150)
    assert state(page)['mode']=='playing',state(page)
    for hp in (2,1,0):
        page.wait_for_timeout(1000)
        for attempt in range(4):
            boss=state(page)
            move(page,boss['bossX']+14,boss['bossY']-58)
            try:
                page.wait_for_function('(h)=>RIKITU_TEST_STATE().bossHP===h',arg=hp,timeout=1400)
                break
            except Exception:
                if attempt==3:raise AssertionError(f'Boss hit failed: {state(page)}')
    move(page,10544,400)
    page.wait_for_function("()=>RIKITU_TEST_STATE().mode==='shop'",timeout=5000)
    before=state(page)
    assert before['bought']==3 and before['maxHearts']==6,before
    # No fourth heart after stage 12: one new unlock per world only.
    page.keyboard.press('b');page.wait_for_timeout(150)
    after=state(page)
    assert after['bought']==3 and after['wallet']==before['wallet'],after
    assert page.evaluate('RIKITU_TEST_SHOP(500,3,12).reason')=='not-unlocked'
    stored=page.evaluate('(key)=>JSON.parse(localStorage.getItem(key))',KEY)
    assert stored['levelIndex']==11 and stored['mode']=='shop' and stored['purchased']==3,stored
    assert len(stored.get('records',[]))>=2,stored
    assert not errors,errors
    print('PASS stages 11-12, crystal spring, sixth boss, shop cap, save migration')
    browser.close()
