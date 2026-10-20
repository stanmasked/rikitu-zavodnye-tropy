"""Complete 16-stage campaign, final boss, epilogue, and 7-heart purchase cap."""
from pathlib import Path
import json,os
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
    for n,(x,y) in enumerate(coords,1):
        move(page,x,y)
        page.wait_for_function('(n)=>RIKITU_TEST_STATE().fragments>=n',arg=n,timeout=5000)
with sync_playwright() as pw:
    exe=os.environ.get('RIKITU_CHROMIUM_PATH') or ('/usr/bin/chromium' if Path('/usr/bin/chromium').is_file() else None)
    browser=pw.chromium.launch(headless=True,executable_path=exe,args=['--no-sandbox'])
    page=browser.new_page(viewport={'width':1200,'height':760})
    errors=[]
    page.on('pageerror',lambda e:errors.append(str(e)))
    old={'version':1,'levelIndex':13,'mode':'victory','wallet':240,'purchased':4,
         'stageCoins':90,'fragments':[0,1,2],'coins':list(range(90)),
         'switches':[],'checkpoint':8690,'hearts':7,'bossDown':True}
    page.evaluate(SHIM,{KEY:json.dumps(old,ensure_ascii=False)})
    page.set_content(HTML,wait_until='load')
    levels=page.evaluate('RIKITU_TEST_LEVELS()')
    assert len(levels)==16 and all(len(d['fragments'])==3 and d['coins']==90 for d in levels)
    assert len(levels[14]['clockPads'])==3 and len(levels[15]['vents'])==3
    assert sum(1 for d in levels if d['boss'] is not None)==8
    page.keyboard.press('Enter')
    page.wait_for_function("()=>RIKITU_TEST_STATE().mode==='between'",timeout=5000)
    page.keyboard.press('Enter')
    page.wait_for_function('()=>RIKITU_TEST_STATE().level===15',timeout=5000)
    assert state(page)['maxHearts']==7
    collect(page,[(1790,351),(5480,350),(9280,349)])
    move(page,10133,400)
    page.wait_for_function("()=>RIKITU_TEST_STATE().mode==='between'",timeout=5000)
    page.keyboard.press('Enter')
    page.wait_for_function("()=>RIKITU_TEST_STATE().level===16",timeout=5000)
    collect(page,[(2010,348),(6190,350),(9940,348)])
    move(page,11574,400)
    page.wait_for_timeout(150)
    assert state(page)['mode']=='playing',state(page)
    for hp in (2,1,0):
        page.wait_for_timeout(1000)
        for attempt in range(4):
            boss=state(page)
            move(page,boss['bossX']+14,boss['bossY']-58)
            try:
                page.wait_for_function('(h)=>RIKITU_TEST_STATE().bossHP===h',arg=hp,timeout=1500)
                break
            except Exception:
                if attempt==3: raise AssertionError(f'Final guardian stomp not registered: {state(page)}')
    move(page,11574,400)
    page.wait_for_function("()=>RIKITU_TEST_STATE().mode==='shop'",timeout=5000)
    before=state(page)
    page.keyboard.press('b');page.wait_for_timeout(120)
    assert state(page)['maxHearts']==7 and state(page)['wallet']==before['wallet']
    page.keyboard.press('Enter')
    page.wait_for_function("()=>RIKITU_TEST_STATE().mode==='victory'",timeout=5000)
    assert len(state(page)['stageRecords'])>=2,state(page)
    saved=page.evaluate('(key)=>JSON.parse(localStorage.getItem(key))',KEY)
    assert saved['mode']=='victory' and saved['levelIndex']==15,saved
    assert not errors,errors
    print('PASS final stages 15-16, eighth guardian, all 16 stages, seven heart cap, ending')
    browser.close()
