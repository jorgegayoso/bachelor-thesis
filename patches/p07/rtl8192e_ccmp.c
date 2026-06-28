#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint16_t __le16;
typedef uint32_t u32;

#define __packed __attribute__((packed))
#define ETH_ALEN 6

#define CCMP_PN_LEN 6

#define le16_to_cpu(x) ((u16)(x))

#define RTLLIB_FCTL_TODS	0x0100
#define RTLLIB_FCTL_FROMDS	0x0200
#define RTLLIB_FTYPE_DATA	0x0008
#define WLAN_FC_GET_TYPE(fc)	((fc) & 0x000c)
#define WLAN_FC_GET_STYPE(fc)	((fc) & 0x00f0)

/* struct rtllib_hdr_4addr. copied from
   drivers/staging/rtl8192e/rtllib.h, with kernel types translated. */
struct rtllib_hdr_4addr {
	__le16 frame_ctl;
	__le16 duration_id;
	u8 addr1[ETH_ALEN];
	u8 addr2[ETH_ALEN];
	u8 addr3[ETH_ALEN];
	__le16 seq_ctl;
	u8 addr4[ETH_ALEN];
	u8 payload[];
} __packed;

/* IOO bug function. copied from
   drivers/staging/rtl8192e/rtllib_crypt_ccmp.c. */
static u8 captured_aad[64];
static int captured_aad_len;

static int ccmp_init_iv_and_aad(struct rtllib_hdr_4addr *hdr,
				u8 *pn, u8 *iv, u8 *aad)
{
	u8 *pos, qc = 0;
	size_t aad_len;
	u16 fc;
	int a4_included, qc_included;

	fc = le16_to_cpu(hdr->frame_ctl);
	a4_included = ((fc & (RTLLIB_FCTL_TODS | RTLLIB_FCTL_FROMDS)) ==
		       (RTLLIB_FCTL_TODS | RTLLIB_FCTL_FROMDS));

	qc_included = ((WLAN_FC_GET_TYPE(fc) == RTLLIB_FTYPE_DATA) &&
		       (WLAN_FC_GET_STYPE(fc) & 0x80));
	aad_len = 22;
	if (a4_included)
		aad_len += 6;
	if (qc_included) {
		pos = (u8 *) &hdr->addr4;
		if (a4_included)
			pos += 6;
		qc = *pos & 0x0f;
		aad_len += 2;
	}
	/* In CCM, the initial vectors (IV) used for CTR mode encryption and CBC
	 * mode authentication are not allowed to collide, yet both are derived
	 * from the same vector. We only set L := 1 here to indicate that the
	 * data size can be represented in (L+1) bytes. The CCM layer will take
	 * care of storing the data length in the top (L+1) bytes and setting
	 * and clearing the other bits as is required to derive the two IVs.
	 */
	iv[0] = 0x1;

	/* Nonce: QC | A2 | PN */
	iv[1] = qc;
	memcpy(iv + 2, hdr->addr2, ETH_ALEN);
	memcpy(iv + 8, pn, CCMP_PN_LEN);

	/* AAD:
	 * FC with bits 4..6 and 11..13 masked to zero; 14 is always one
	 * A1 | A2 | A3
	 * SC with bits 4..15 (seq#) masked to zero
	 * A4 (if present)
	 * QC (if present)
	 */
	pos = (u8 *) hdr;
	aad[0] = pos[0] & 0x8f;
	aad[1] = pos[1] & 0xc7;
	memcpy(aad + 2, hdr->addr1, 3 * ETH_ALEN);
	pos = (u8 *) &hdr->seq_ctl;
	aad[20] = pos[0] & 0x0f;
	aad[21] = 0; /* all bits masked */
	memset(aad + 22, 0, 8);
	if (a4_included)
		memcpy(aad + 22, hdr->addr4, ETH_ALEN);
	if (qc_included) {
		aad[a4_included ? 28 : 22] = qc;
		/* rest of QC masked */
	}

	memcpy(captured_aad, aad, sizeof(captured_aad));
	captured_aad_len = (int)aad_len;
	return aad_len;
}
