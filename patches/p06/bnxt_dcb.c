#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>

typedef uint8_t  u8;
typedef uint32_t __le32;

#define IEEE_8021QAZ_TSA_STRICT                              1
#define IEEE_8021QAZ_TSA_ETS                                 2
#define QUEUE_COS2BW_QCFG_RESP_QUEUE_ID0_TSA_ASSIGN_SP       1
#define HWRM_QUEUE_COS2BW_QCFG                               0
#define HWRM_CMD_TIMEOUT                                     0

/* buggy struct. copied from drivers/net/ethernet/broadcom/bnxt/bnxt_dcb.h */
struct bnxt_cos2bw_cfg {
	u8			pad[3];
	u8			queue_id;
	__le32			min_bw;
	__le32			max_bw;
#define BW_VALUE_UNIT_PERCENT1_100		(0x1UL << 29)
	u8			tsa;
	u8			pri_lvl;
	u8			bw_weight;
	u8			unused;
};

struct hwrm_queue_cos2bw_qcfg_output {
	u8	_padding[16];
	u8	queue_id0;
	u8	_trailing[256];
};

struct hwrm_queue_cos2bw_qcfg_input { u8 _stub[64]; };

struct ieee_ets {
	u8	tc_tsa[8];
	u8	tc_tx_bw[8];
};

struct bnxt {
	void	*hwrm_cmd_resp_addr;
	int	hwrm_cmd_lock;	/* unused, macro discards arg */
	int	max_tc;
};

static void bnxt_hwrm_cmd_hdr_init(struct bnxt *bp, void *req,
				   int t, int a, int b) { (void)bp;(void)req; }
static int  _hwrm_send_message(struct bnxt *bp, void *req, size_t s, int t)
{ (void)bp;(void)req;(void)s;(void)t; return 0; }
static int  bnxt_queue_to_tc(struct bnxt *bp, u8 q)
{ (void)bp; return q & 0x07; }
#define mutex_lock(x)	((void)0)
#define mutex_unlock(x)	((void)0)

static struct bnxt_cos2bw_cfg captured;

/* IOO bug function. copied from
   drivers/net/ethernet/broadcom/bnxt/bnxt_dcb.c */
static int bnxt_hwrm_queue_cos2bw_qcfg(struct bnxt *bp, struct ieee_ets *ets)
{
	struct hwrm_queue_cos2bw_qcfg_output *resp = bp->hwrm_cmd_resp_addr;
	struct hwrm_queue_cos2bw_qcfg_input req = {0};
	struct bnxt_cos2bw_cfg cos2bw;
	void *data;
	int rc, i;

	bnxt_hwrm_cmd_hdr_init(bp, &req, HWRM_QUEUE_COS2BW_QCFG, -1, -1);

	mutex_lock(&bp->hwrm_cmd_lock);
	rc = _hwrm_send_message(bp, &req, sizeof(req), HWRM_CMD_TIMEOUT);
	if (rc) {
		mutex_unlock(&bp->hwrm_cmd_lock);
		return rc;
	}

	data = &resp->queue_id0 + offsetof(struct bnxt_cos2bw_cfg, queue_id);
	for (i = 0; i < bp->max_tc; i++, data += sizeof(cos2bw) - 4) {
		int tc;

		memset(&cos2bw, 0xAA, sizeof(cos2bw));
		/* buggy line, A1 */
		memcpy(&cos2bw.queue_id, data, sizeof(cos2bw) - 4);
		if (i == 0)
			cos2bw.queue_id = resp->queue_id0;

		tc = bnxt_queue_to_tc(bp, cos2bw.queue_id);
		if (tc < 0)
			continue;

		if (cos2bw.tsa ==
		    QUEUE_COS2BW_QCFG_RESP_QUEUE_ID0_TSA_ASSIGN_SP) {
			ets->tc_tsa[tc] = IEEE_8021QAZ_TSA_STRICT;
		} else {
			ets->tc_tsa[tc] = IEEE_8021QAZ_TSA_ETS;
			ets->tc_tx_bw[tc] = cos2bw.bw_weight;
		}
	}
	mutex_unlock(&bp->hwrm_cmd_lock);

	captured = cos2bw;	/* expose final iteration's cos2bw for main() */
	return 0;
}
