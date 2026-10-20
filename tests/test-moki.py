"""Exercise actual Moki shop purchases through the browser UI; no edited game data."""
from pathlib import Path
import os
from playwright.sync_api import sync_playwright

ROOT=Path(__file__).resolve().parents[1]
FRAGS=[
    [(1520,354),(3640,339),(6860,354)],
    [(1640,348),(4265,346),(7020,350)],
    [(1520,350),(3960,349),(7030,350)],
    [(1570,349),(4580,348),(7410,350)],
]
GATES=[7790,8390,8160,8750]

def state(page):
    return page.evaluate('RIKITU_TEST_STATE()')

def collect_parts(page,i):
    for count,(x,y) in enumerate(FRAGS[i],start=1):
        page.evaluate('(pos)=>RIKITU_TEST_MOVE(...pos)',[x,y])
        page.wait_for_function('(expected)=>RIKITU_TEST_STATE().fragments>=expected',arg=count,timeout=4000)
    assert state(page)['fragments']==3,state(page)

def beat_boss(page):
    for hp in (2,1,0):
        # Wait for the boss's invulnerable cooldown, then use a controlled stomp.
        page.wait_for_timeout(1000)
        for attempt in range(4):
            s=state(page)
            page.evaluate('(p)=>RIKITU_TEST_MOVE(p[0]+14,p[1]-58)',[s['bossX'],s['bossY']])
            try:
                page.wait_for_function('(expected)=>RIKITU_TEST_STATE().bossHP===expected',arg=hp,timeout=900)
                break
            except Exception:
                if attempt==3:raise AssertionError(f'Boss stomp not registered: {state(page)}')
        assert state(page)['bossHP']==hp,state(page)

def finish_level(page,i):
    collect_parts(page,i)
    if i in (1,3):
        beat_boss(page)
    page.evaluate('(pos)=>RIKITU_TEST_MOVE(...pos)',[GATES[i],400])
    page.wait_for_function('(expected)=>RIKITU_TEST_STATE().mode===expected',
                           arg=('shop' if i in (1,3) else 'between'),timeout=5000)
    
def press_next(page):
    page.keyboard.press('Enter')
    page.wait_for_timeout(125)

with sync_playwright() as p:
    path=os.environ.get('RIKITU_CHROMIUM_PATH') or ('/usr/bin/chromium' if Path('/usr/bin/chromium').exists() else None)
    browser=p.chromium.launch(headless=True,executable_path=path,args=['--no-sandbox'])
    page=browser.new_page(viewport={'width':1200,'height':760})
    errors=[]
    page.on('pageerror',lambda e:errors.append(str(e)))
    html=(ROOT/'RIKITU_play_now.html').read_text(encoding='utf-8')

    # Scenario A: buy immediately after stage 2; cannot buy the 5th heart yet.
    page.set_content(html,wait_until='load')
    press_next(page)
    assert state(page)['level']==1
    assert page.evaluate('RIKITU_TEST_COLLECT_COINS(90)')
    assert state(page)['wallet']==90
    assert page.evaluate('RIKITU_TEST_COLLECT_COINS(90)')
    assert state(page)['wallet']==90 # pickups cannot be claimed twice
    finish_level(page,0)
    press_next(page)
    assert state(page)['level']==2
    assert page.evaluate('RIKITU_TEST_COLLECT_COINS(90)')
    finish_level(page,1)
    assert state(page)['wallet']==180
    page.keyboard.press('b')
    page.wait_for_timeout(100)
    purchased=state(page)
    assert purchased['wallet']==60 and purchased['maxHearts']==4 and purchased['bought']==1, purchased
    page.keyboard.press('b')
    page.wait_for_timeout(100)
    assert state(page)['wallet']==60 and state(page)['bought']==1
    # A fresh page avoids duplicate event handlers and animation loops across scenarios.
    page.close()
    page=browser.new_page(viewport={'width':1200,'height':760})
    page.on('pageerror',lambda e:errors.append(str(e)))

    # Scenario B: skip first shop, then buy its carried-over upgrade after stage 4.
    page.set_content(html,wait_until='load')
    press_next(page)
    page.evaluate('RIKITU_TEST_COLLECT_COINS(90)')
    finish_level(page,0)
    press_next(page)
    page.evaluate('RIKITU_TEST_COLLECT_COINS(90)')
    finish_level(page,1)
    press_next(page)
    assert state(page)['mode']=='between' and state(page)['maxHearts']==3
    press_next(page)
    assert state(page)['level']==3
    finish_level(page,2)
    press_next(page)
    assert state(page)['level']==4
    finish_level(page,3)
    before=state(page)
    assert before['mode']=='shop' and before['wallet']>=180, before
    # Optional coins collected while reaching fragments are legitimate income.
    page.keyboard.press('b')
    page.wait_for_timeout(130)
    final=state(page)
    assert final['maxHearts']==4 and final['bought']==1 and final['wallet']==before['wallet']-120, final
    page.keyboard.press('b')
    page.wait_for_timeout(90)
    assert state(page)['bought']==1
    assert not errors, errors
    print('PASS Moki shop: wallet debit, heart cap, deferred upgrade, double-purchase guard')
    browser.close()
