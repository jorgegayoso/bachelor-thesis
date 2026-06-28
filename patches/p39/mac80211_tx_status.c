#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <stdbool.h>

typedef int8_t   s8;
typedef int32_t  s32;
typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef u64 codel_time_t;

#define __packed __attribute__((packed))
#define BUILD_BUG_ON(x) ((void)0)
#define WARN_ON(x)      ((void)(x))

#define IEEE80211_TX_INFO_DRIVER_DATA_SIZE       40
#define IEEE80211_TX_INFO_RATE_DRIVER_DATA_SIZE  24
#define IEEE80211_TX_MAX_RATES                    4

struct ieee80211_tx_rate {
	s8 idx;
	u16 count:5,
	    flags:11;
} __packed;

struct ieee80211_vif      { int dummy; };
struct ieee80211_key_conf { int dummy; };

struct ieee80211_tx_info {
	/* common information */
	u32 flags;
	u32 band:3,
	    ack_frame_id:13,
	    hw_queue:4,
	    tx_time_est:10;
	/* 2 free bits */

	union {
		struct {
			union {
				/* rate control */
				struct {
					struct ieee80211_tx_rate rates[
						IEEE80211_TX_MAX_RATES];
					s8 rts_cts_rate_idx;
					u8 use_rts:1;
					u8 use_cts_prot:1;
					u8 short_preamble:1;
					u8 skip_table:1;
					/* 2 bytes free */
				};
				/* only needed before rate control */
				unsigned long jiffies;
			};
			/* NB: vif can be NULL for injected frames */
			struct ieee80211_vif *vif;
			struct ieee80211_key_conf *hw_key;
			u32 flags;
			codel_time_t enqueue_time;
		} control;
		struct {
			u64 cookie;
		} ack;
		struct {
			struct ieee80211_tx_rate rates[IEEE80211_TX_MAX_RATES];
			s32 ack_signal;
			u8 ampdu_ack_len;
			u8 ampdu_len;
			u8 antenna;
			u16 tx_time;
			bool is_valid_ack_signal;
			void *status_driver_data[19 / sizeof(void *)];
		} status;
		struct {
			struct ieee80211_tx_rate driver_rates[
				IEEE80211_TX_MAX_RATES];
			u8 pad[4];

			void *rate_driver_data[
				IEEE80211_TX_INFO_RATE_DRIVER_DATA_SIZE / sizeof(void *)];
		};
		void *driver_data[
			IEEE80211_TX_INFO_DRIVER_DATA_SIZE / sizeof(void *)];
	};
};

/* IOO bug function. copied from
   include/net/mac80211.h */
static inline void
ieee80211_tx_info_clear_status(struct ieee80211_tx_info *info)
{
	int i;

	BUILD_BUG_ON(offsetof(struct ieee80211_tx_info, status.rates) !=
		     offsetof(struct ieee80211_tx_info, control.rates));
	BUILD_BUG_ON(offsetof(struct ieee80211_tx_info, status.rates) !=
		     offsetof(struct ieee80211_tx_info, driver_rates));
	BUILD_BUG_ON(offsetof(struct ieee80211_tx_info, status.rates) != 8);
	/* clear the rate counts */
	for (i = 0; i < IEEE80211_TX_MAX_RATES; i++)
		info->status.rates[i].count = 0;

	BUILD_BUG_ON(
	    offsetof(struct ieee80211_tx_info, status.ack_signal) != 20);
	
	/* buggy line, B2 */
	memset(&info->status.ampdu_ack_len, 0,
	       sizeof(struct ieee80211_tx_info) -
	       offsetof(struct ieee80211_tx_info, status.ampdu_ack_len));
}
