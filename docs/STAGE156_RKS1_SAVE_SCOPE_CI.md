# Stage156 — MAP01-only RKS1 save-scope CI

The Stage155 production change restricts legacy RKS1 save writes to MAP01.
MAP02–MAP16 use their own RKL3 level format and RKC2 campaign state;
diagnostic Y/X sessions must never write permanent save slots.

Stage156 adds a **dedicated synthetic-only regression workflow** for that
contract. Its GCC, Clang and sanitizer checks run native campaign progression
across all 16 levels and verify legacy-checkpoint eligibility remains true
only for chapter 1, even at shop/between-level transitions. The independent
Node source check verifies all three production RKS1 `store_pair` callsites
are guarded by the campaign and level identity, and that debug sessions
cannot modify them.

This is a supplement, not a replacement, for full graphical Switch/NeXium
save recovery checks. A successful host test does **not** prove power-loss
durability on Switch SD storage. Any future change to save paths, write
ordering or transaction behavior still requires independent guest and
hardware verification.

No ROM, original imagery, screenshots, real saved game state or other
private data enters the public workflow. The permanent raw-capture archive
policy remains in effect.
