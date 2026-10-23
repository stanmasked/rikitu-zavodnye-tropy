# Stage150 — five-camera MAP01 composite regression

Status: **synthetic / technical only**, never an approval of private GFX-01 art.

The source MAP01 world is 7,900 game pixels wide. The draft panorama geometry contract spans 5,120 source pixels at a 1,280 × 720 framebuffer and maps the clamped camera range of 0–6,620 to source offsets 0–3,840.

The Stage150 host test calls the **real** `rikitu_flower_init` (19 solids, 90 coins and native gate/checkpoint positions), but uses a procedurally generated opaque synthetic RGBA8888 panorama, not the private GFX-01 art. It tests five representative camera positions: 0, 1,655, 3,310, 4,965, 6,620, expecting source crop origins 0, 960, 1,920, 2,880, 3,840.

For every camera position, the test verifies that:

- The bounded background blit selects the expected crop within ±1 pixel of floating-point projection.
- The optional overlay preserves background sky pixels (so debug backdrops never cover a prefilled source), and existing HUD/character-hitbox pixels are drawn above it.
- Source and destination row padding sentinels remain unchanged, and the original MAP01 solids/coins counts remain fixed.

After five frames, the same test calls the original `rikitu_draw_scene` to verify the default synthetic sky still paints normally. GCC, Clang, Clang ASan/UBSan execute the synthetic test; no framebuffer image is uploaded to public CI.

**Important:** These are source-independent pixels with neutral hitboxes, not gameplay captures and not artistic approval. Neither canonical Rikitu/Moki/Reina art nor the privately archived revised flower-trail SVG is loaded. Actual Switch/NeXium graphical integration remains gated on GFX-02 (#126): human approval and independently verified private source/readback, texture loading and private runtime capture QA.
