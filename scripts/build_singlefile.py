#!/usr/bin/env python3
from pathlib import Path
import sys
r=Path(__file__).resolve().parent.parent
s=(r/'index.html').read_text(encoding='utf-8')
for old,new in [
 ('<link rel="stylesheet" href="style.css"/>','<style>\n'+(r/'style.css').read_text(encoding='utf-8')+'\n</style>'),
 ('<script src="game-core.js"></script>','<script>\n'+(r/'game-core.js').read_text(encoding='utf-8')+'\n</script>'),
 ('<script src="game.js"></script>','<script>\n'+(r/'game.js').read_text(encoding='utf-8')+'\n</script>')]:
    assert s.count(old)==1,old
    s=s.replace(old,new,1)
target=r/'RIKITU_play_now.html'
if '--check' in sys.argv:
    assert target.read_text(encoding='utf-8')==s,'Standalone HTML out of date'
    print('PASS standalone HTML up to date')
else:
    target.write_text(s,encoding='utf-8')
    print('Wrote',target)
