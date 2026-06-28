#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>

typedef uint8_t  __u8;
typedef uint16_t __u16;
typedef uint16_t __be16;
typedef uint32_t __be32;

#define htonl(x) ((__be32)(x))
#define htons(x) ((__be16)(x))

#define HIPPI_ALEN         6
#define HIPPI_OUI_LEN      3
#define HIPPI_EXTENDED_SAP 0xAA
#define HIPPI_UI_CMD       0x03

struct hippi_fp_hdr {
	__be32		fixed;
	__be32		d2_size;
} __attribute__((packed));

struct hippi_le_hdr {
	__u8		message_type:4;
	__u8		double_wide:1;
	__u8		fc:3;
	__u8		dest_switch_addr[3];
	__u8		src_addr_type:4,
			dest_addr_type:4;
	__u8		src_switch_addr[3];
	__u16		reserved;
	__u8		daddr[HIPPI_ALEN];
	__u16		locally_administered;
	__u8		saddr[HIPPI_ALEN];
} __attribute__((packed));

struct hippi_snap_hdr {
	__u8	dsap;			/* always 0xAA */
	__u8	ssap;			/* always 0xAA */
	__u8	ctrl;			/* always 0x03 */
	__u8	oui[HIPPI_OUI_LEN];	/* organizational universal id (zero)*/
	__be16	ethertype;		/* packet type ID field */
} __attribute__((packed));

struct hippi_hdr {
	struct hippi_fp_hdr	fp;
	struct hippi_le_hdr	le;
	struct hippi_snap_hdr	snap;
} __attribute__((packed));

#define HIPPI_HLEN sizeof(struct hippi_hdr)

struct sk_buff {
	__u8 cb[64];
	int  len;
	__u8 storage[256];
	int  pushed;
};
struct net_device { __u8 dev_addr[16]; };
struct hippi_cb { __u32 ifield; };

static void *skb_push(struct sk_buff *skb, unsigned int len)
{
	skb->pushed = (int)len;
	return &skb->storage[0];
}

/* IOO bug function. copied from
   net/802/hippi.c */
static int hippi_header(struct sk_buff *skb, struct net_device *dev,
			unsigned short type,
			const void *daddr, const void *saddr, unsigned int len)
{
	struct hippi_hdr *hip = skb_push(skb, HIPPI_HLEN);
	struct hippi_cb *hcb = (struct hippi_cb *) skb->cb;

	if (!len){
		len = skb->len - HIPPI_HLEN;
		printf("hippi_header(): length not supplied\n");
	}

	/*
	 * Due to the stupidity of the little endian byte-order we
	 * have to set the fp field this way.
	 */
	hip->fp.fixed		= htonl(0x04800018);
	hip->fp.d2_size		= htonl(len + 8);
	hip->le.fc		= 0;
	hip->le.double_wide	= 0;	/* only HIPPI 800 for the time being */
	hip->le.message_type	= 0;	/* Data PDU */

	hip->le.dest_addr_type	= 2;	/* 12 bit SC address */
	hip->le.src_addr_type	= 2;	/* 12 bit SC address */

	memcpy(hip->le.src_switch_addr, dev->dev_addr + 3, 3);
	/* buggy line, B2 */
	memset(&hip->le.reserved, 0, 16);

	hip->snap.dsap		= HIPPI_EXTENDED_SAP;
	hip->snap.ssap		= HIPPI_EXTENDED_SAP;
	hip->snap.ctrl		= HIPPI_UI_CMD;
	hip->snap.oui[0]	= 0x00;
	hip->snap.oui[1]	= 0x00;
	hip->snap.oui[2]	= 0x00;
	hip->snap.ethertype	= htons(type);

	if (daddr)
	{
		memcpy(hip->le.dest_switch_addr, daddr + 3, 3);
		memcpy(&hcb->ifield, daddr + 2, 4);
		return HIPPI_HLEN;
	}
	hcb->ifield = 0;
	return -((int)HIPPI_HLEN);
}
