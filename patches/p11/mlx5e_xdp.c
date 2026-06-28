#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <stdbool.h>

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint16_t __be16;
typedef uint32_t u32;
typedef uint32_t __be32;
typedef uint64_t __be64;
typedef uint64_t dma_addr_t;

#define unlikely(x) (x)
#define cpu_to_be16(x) ((__be16)(x))
#define cpu_to_be32(x) ((__be32)(x))
#define cpu_to_be64(x) ((__be64)(x))

#define INDIRECT_CALLABLE_SCOPE static
#define net_prefetchw(x)   ((void)(x))

#define MLX5_INLINE_MODE_NONE 0
#define MLX5_OPCODE_SEND      0xa

#define MLX5E_XDP_MIN_INLINE 18

struct mlx5_wqe_ctrl_seg {
	__be32			opmod_idx_opcode;
	__be32			qpn_ds;
	u8			signature;
	u8			rsvd[2];
	u8			fm_ce_se;
	union {
		__be32		general_id;
		__be32		imm;
		__be32		umr_mkey;
		__be32		tis_tir_num;
	};
};

struct mlx5_wqe_eth_seg {
	u8              swp_outer_l4_offset;
	u8              swp_outer_l3_offset;
	u8              swp_inner_l4_offset;
	u8              swp_inner_l3_offset;
	u8              cs_flags;
	u8              swp_flags;
	__be16          mss;
	__be32          flow_table_metadata;
	union {
		struct {
			__be16 sz;
			u8     start[2];
		} inline_hdr;
		struct {
			__be16 type;
			__be16 vlan_tci;
		} insert;
		__be32 trailer;
	};
};

struct mlx5_wqe_data_seg {
	__be32			byte_count;
	__be32			lkey;
	__be64			addr;
};

struct mlx5e_tx_wqe {
	struct mlx5_wqe_ctrl_seg ctrl;
	struct mlx5_wqe_eth_seg  eth;
	struct mlx5_wqe_data_seg data[0];
};

struct mlx5e_xmit_data {
	dma_addr_t  dma_addr;
	void       *data;
	u32         len;
};

struct mlx5e_xdp_info { int dummy; };

struct mlx5e_xdpsq_stats { u32 err, full, xmit; };
struct mlx5_wq_cyc { int sz; };
struct mlx5e_xdpsq {
	struct mlx5_wq_cyc        wq;
	u32                       cc, pc;
	int                       min_inline_mode;
	u32                       hw_mtu;
	struct mlx5e_xdpsq_stats *stats;
	void                     *doorbell_cseg;
	struct {
		struct {
			int dummy;
		} xdpi_fifo;
	} db;
};

static struct mlx5e_tx_wqe *g_wqe;

static inline u16 mlx5_wq_cyc_ctr2ix(struct mlx5_wq_cyc *wq, u32 ctr)
{ (void)wq; return (u16)ctr; }
static inline struct mlx5e_tx_wqe *mlx5_wq_cyc_get_wqe(struct mlx5_wq_cyc *wq,
						       u16 pi)
{ (void)wq; (void)pi; return g_wqe; }

static inline int mlx5e_xmit_xdp_frame_check(struct mlx5e_xdpsq *sq)
{ (void)sq; return 1; }

static inline void mlx5e_xdpi_fifo_push(void *fifo, struct mlx5e_xdp_info *xdpi)
{ (void)fifo; (void)xdpi; }

/* IOO bug function. copied from
   drivers/net/ethernet/mellanox/mlx5/core/en/xdp.c */
INDIRECT_CALLABLE_SCOPE bool
mlx5e_xmit_xdp_frame(struct mlx5e_xdpsq *sq, struct mlx5e_xmit_data *xdptxd,
		     struct mlx5e_xdp_info *xdpi, int check_result)
{
	struct mlx5_wq_cyc       *wq   = &sq->wq;
	u16                       pi   = mlx5_wq_cyc_ctr2ix(wq, sq->pc);
	struct mlx5e_tx_wqe      *wqe  = mlx5_wq_cyc_get_wqe(wq, pi);

	struct mlx5_wqe_ctrl_seg *cseg = &wqe->ctrl;
	struct mlx5_wqe_eth_seg  *eseg = &wqe->eth;
	struct mlx5_wqe_data_seg *dseg = wqe->data;

	dma_addr_t dma_addr = xdptxd->dma_addr;
	u32 dma_len = xdptxd->len;

	struct mlx5e_xdpsq_stats *stats = sq->stats;

	net_prefetchw(wqe);

	if (unlikely(dma_len < MLX5E_XDP_MIN_INLINE || sq->hw_mtu < dma_len)) {
		stats->err++;
		return false;
	}

	if (!check_result)
		check_result = mlx5e_xmit_xdp_frame_check(sq);
	if (unlikely(check_result < 0))
		return false;

	cseg->fm_ce_se = 0;

	/* copy the inline part if required */
	if (sq->min_inline_mode != MLX5_INLINE_MODE_NONE) {
		/* buggy line, A3*/
		memcpy(eseg->inline_hdr.start, xdptxd->data, MLX5E_XDP_MIN_INLINE);
		eseg->inline_hdr.sz = cpu_to_be16(MLX5E_XDP_MIN_INLINE);
		dma_len  -= MLX5E_XDP_MIN_INLINE;
		dma_addr += MLX5E_XDP_MIN_INLINE;
		dseg++;
	}

	/* write the dma part */
	dseg->addr       = cpu_to_be64(dma_addr);
	dseg->byte_count = cpu_to_be32(dma_len);

	cseg->opmod_idx_opcode = cpu_to_be32((sq->pc << 8) | MLX5_OPCODE_SEND);

	sq->pc++;

	sq->doorbell_cseg = cseg;

	mlx5e_xdpi_fifo_push(&sq->db.xdpi_fifo, xdpi);
	stats->xmit++;
	return true;
}
