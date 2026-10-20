#ifndef RIKITU_RULES_H
#define RIKITU_RULES_H
/* Portable C99 rule kernel. No game assets, platform SDK or allocation. */
#ifdef __cplusplus
extern "C" {
#endif
enum { RIKITU_STARTING_HEARTS = 3, RIKITU_MAX_HEARTS = 7,
       RIKITU_STAGE_COINS = 90, RIKITU_REQUIRED_FRAGMENTS = 3 };
typedef struct RikituRect { float x, y, w, h; } RikituRect;
typedef enum RikituOfferReason {
  RIKITU_OFFER_READY = 0,
  RIKITU_OFFER_INSUFFICIENT_COINS = 1,
  RIKITU_OFFER_NOT_UNLOCKED = 2,
  RIKITU_OFFER_NOT_IN_SHOP_OR_MAXED = 3
} RikituOfferReason;
typedef struct RikituHeartOffer {
  int available, price, hearts_after, unlocked;
  RikituOfferReason reason;
} RikituHeartOffer;
typedef struct RikituHeartPurchase {
  int success, purchased_count, coins, hearts;
} RikituHeartPurchase;
int rikitu_box_hit(RikituRect a, RikituRect b);
int rikitu_can_exit(int fragments, int boss_required, int boss_defeated);
int rikitu_can_jump(int grounded, float coyote_seconds);
int rikitu_stomp_from_above(float prev_bottom, float now_bottom,
                            float target_top, int falling);
int rikitu_heart_after_damage(int hearts);
int rikitu_is_shop_stage(int stage);
int rikitu_unlocked_upgrades(int cleared_stage);
RikituHeartOffer rikitu_next_heart_offer(int cleared_stage,
                                        int purchased_count, int coins);
RikituHeartPurchase rikitu_buy_heart(int cleared_stage,
                                    int purchased_count, int coins);
#ifdef __cplusplus
}
#endif
#endif
