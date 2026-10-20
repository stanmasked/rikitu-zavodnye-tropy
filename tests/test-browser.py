from pathlib import Path
import os
from playwright.sync_api import sync_playwright
ROOT=Path(__file__).resolve().parents[1]
with sync_playwright() as p:
    binary=os.environ.get('RIKITU_CHROMIUM_PATH') or ('/usr/bin/chromium' if Path('/usr/bin/chromium').is_file() else None)
    browser=p.chromium.launch(headless=True,executable_path=binary,args=['--no-sandbox'])
    page=browser.new_page(viewport={'width':1200,'height':760})
    errors=[];page.on('pageerror',lambda err:errors.append(str(err)))
    page.set_content((ROOT/'RIKITU_play_now.html').read_text(encoding='utf-8'),wait_until='load')
    page.keyboard.press('Enter');page.wait_for_timeout(160)
    assert page.evaluate('RIKITU_TEST_STATE().mode')=='playing'
    a=page.evaluate('RIKITU_TEST_LEVELS()')
    assert len(a)==2 and a[0]['coins']==90 and a[1]['coins']==90
    assert a[0]['width']>=7900 and a[1]['width']>=8550
    assert len(a[0]['checkpoints'])==3 and len(a[0]['sections'])==4
    for x,y in [(1520,354),(3640,339),(6860,354)]:
        page.evaluate('(p)=>RIKITU_TEST_MOVE(...p)',[x,y]);page.wait_for_timeout(75)
    assert page.evaluate('RIKITU_TEST_STATE().fragments')==3
    page.evaluate('RIKITU_TEST_MOVE(7790,400)');page.wait_for_timeout(100)
    assert page.evaluate('RIKITU_TEST_STATE().mode')=='between'
    page.keyboard.press('Enter');page.wait_for_timeout(100)
    for x,y in [(1640,348),(4265,346),(7020,350)]:
        page.evaluate('(p)=>RIKITU_TEST_MOVE(...p)',[x,y]);page.wait_for_timeout(80)
    assert page.evaluate('RIKITU_TEST_STATE().fragments')==3
    page.evaluate('RIKITU_TEST_MOVE(8390,400)');page.wait_for_timeout(90)
    assert page.evaluate('RIKITU_TEST_STATE().mode')=='playing'
    for expected in [2,1,0]:
        page.wait_for_timeout(920)
        state=page.evaluate('RIKITU_TEST_STATE()')
        page.evaluate('(p)=>RIKITU_TEST_MOVE(p[0]+14,p[1]-58)',[state['bossX'],state['bossY']]);page.wait_for_timeout(350)
        assert page.evaluate('RIKITU_TEST_STATE().bossHP')==expected
    page.evaluate('RIKITU_TEST_MOVE(8390,400)');page.wait_for_timeout(90)
    assert page.evaluate('RIKITU_TEST_STATE().mode')=='shop'
    assert page.evaluate('RIKITU_TEST_SHOP(120,0,2).available')
    page.keyboard.press('Enter');page.wait_for_timeout(70)
    assert page.evaluate('RIKITU_TEST_STATE().mode')=='victory'
    assert not errors,errors
    print('PASS browser route, fragment gate, mini boss, hamster shop')
    browser.close()
