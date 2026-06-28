#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint16_t __le16;
typedef uint32_t u32;
typedef uint32_t __le32;

#define __packed __attribute__((packed))
#define cpu_to_le16(x) ((u16)(x))
#define le16_to_cpu(x) ((u16)(x))

#define IEEE80211_HT_MCS_MASK_LEN	10

/* struct ieee80211_mcs_info. copied from include/linux/ieee80211.h. */
struct ieee80211_mcs_info {
	u8	rx_mask[IEEE80211_HT_MCS_MASK_LEN];
	__le16	rx_highest;
	u8	tx_params;
	u8	reserved[3];
} __packed;

/* struct ieee80211_ht_cap. copied from include/linux/ieee80211.h. */
struct ieee80211_ht_cap {
	__le16	cap_info;
	u8	ampdu_params_info;
	struct ieee80211_mcs_info mcs;
	__le16	extended_ht_cap_info;
	__le32	tx_BF_cap_info;
	u8	antenna_selection_info;
} __packed;

struct adapter { int dummy; };
struct mlme_ext_priv { u8 default_supported_mcs_set[16]; };

/* IOO bug function. copied from
   drivers/staging/rtl8723bs/core/rtw_mlme.c */
static void rtw_restructure_ht_ie_slice(struct mlme_ext_priv *pmlmeext)
{
	struct ieee80211_ht_cap ht_capie;

	memset(&ht_capie, 0xAA, sizeof(struct ieee80211_ht_cap));

	/* fill default supported_mcs_set */
	/* buggy line, A2 */
	memcpy(ht_capie.mcs.rx_mask, pmlmeext->default_supported_mcs_set, 16);
}
