#ifndef RIKITU_CAMPAIGN_H
#define RIKITU_CAMPAIGN_H
/* Campaign state and Moki shop, independent of libnx and screen rendering. */
#include "rikitu_stage.h"
typedef enum RikituCampaignMode {
  RIKITU_CAMPAIGN_PLAYING=1,RIKITU_CAMPAIGN_BETWEEN=2,
  RIKITU_CAMPAIGN_SHOP=3,RIKITU_CAMPAIGN_VICTORY=4
} RikituCampaignMode;
typedef struct RikituCampaign {
  int level;
  RikituCampaignMode mode;
  int wallet,purchased_hearts,max_hearts;
} RikituCampaign;
int rikitu_campaign_init(RikituCampaign *c,int level);
int rikitu_campaign_valid(const RikituCampaign *c);
/* RKS1 is reserved for the legacy MAP01 intra-level checkpoint. */
int rikitu_campaign_uses_legacy_map01_checkpoint(const RikituCampaign *c);
int rikitu_campaign_finish(RikituCampaign *c,const RikituStage *stage);
/* Returns 1 on next level, 2 for final victory, 0 for no transition. */
int rikitu_campaign_continue(RikituCampaign *c);
RikituHeartOffer rikitu_campaign_offer(const RikituCampaign *c);
RikituHeartPurchase rikitu_campaign_buy(RikituCampaign *c);
#endif
