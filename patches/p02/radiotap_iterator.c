#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>

typedef uint8_t  __u8;
typedef uint16_t __le16;
typedef uint32_t __le32;
typedef uint16_t __u16;
typedef uint32_t __u32;

#define __packed __attribute__((packed))

#define EINVAL 22

#define get_unaligned_le16(p) ((__le16)(*(const __u16 *)(p)))
#define get_unaligned_le32(p) ((__le32)(*(const __u32 *)(p)))

struct ieee80211_radiotap_header {
	uint8_t it_version;
	uint8_t it_pad;
	__le16 it_len;
	__le32 it_present;
} __packed;

struct ieee80211_radiotap_namespace            { int dummy; };
struct ieee80211_radiotap_vendor_namespaces    { int dummy; };

static const struct ieee80211_radiotap_namespace radiotap_ns;

struct ieee80211_radiotap_iterator {
	struct ieee80211_radiotap_header *_rtheader;
	const struct ieee80211_radiotap_vendor_namespaces *_vns;
	const struct ieee80211_radiotap_namespace *current_namespace;

	unsigned char *_arg;
	__le32 *_next_bitmap;

	int is_radiotap_ns;
	int _max_length;
	int _arg_index;
	uint32_t _bitmap_shifter;
	int _reset_on_ext;
};

/* IOO bug function. copied from
   net/wireless/radiotap.c */
int ieee80211_radiotap_iterator_init(
	struct ieee80211_radiotap_iterator *iterator,
	struct ieee80211_radiotap_header *radiotap_header,
	int max_length, const struct ieee80211_radiotap_vendor_namespaces *vns)
{
	/* check the radiotap header can actually be present */
	if (max_length < (int)sizeof(struct ieee80211_radiotap_header))
		return -EINVAL;

	/* Linux only supports version 0 radiotap format */
	if (radiotap_header->it_version)
		return -EINVAL;

	/* sanity check for allowed length and radiotap length field */
	if (max_length < get_unaligned_le16(&radiotap_header->it_len))
		return -EINVAL;

	iterator->_rtheader = radiotap_header;
	iterator->_max_length = get_unaligned_le16(&radiotap_header->it_len);
	iterator->_arg_index = 0;
	iterator->_bitmap_shifter = get_unaligned_le32(&radiotap_header->it_present);
	iterator->_arg = (uint8_t *)radiotap_header + sizeof(*radiotap_header);
	iterator->_reset_on_ext = 0;

	/* buggy lines, C */
	iterator->_next_bitmap = &radiotap_header->it_present;
	iterator->_next_bitmap++;

	iterator->_vns = vns;
	iterator->current_namespace = &radiotap_ns;
	iterator->is_radiotap_ns = 1;

	return 0;
}