#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

/* SRP wire size copied from viosrp.h */
#define SRP_MAX_IU_LEN 256
#define SRP_CMD 0x02
#define SCSI_MLQUEUE_HOST_BUSY 1
#define DID_OK 0x00

struct scsi_lun { u8 scsi_lun[8]; };

/* struct srp_cmd. copied from include/scsi/srp.h, kernel types
   translated. */
struct srp_cmd {
	u8	opcode;
	u8	sol_not;
	u8	reserved1[3];
	u8	buf_fmt;
	u8	data_out_desc_cnt;
	u8	data_in_desc_cnt;
	u64	tag;
	u8	reserved2[4];
	struct scsi_lun	lun;
	u8	reserved3;
	u8	task_attr;
	u8	reserved4;
	u8	add_cdb_len;
	u8	cdb[16];
	u8	add_data[];
};

struct srp_login_req { u8 _[64]; };
struct srp_login_rsp { u8 _[52]; };
struct srp_login_rej { u8 _[16]; };
struct srp_t_logout  { u8 _[16]; };
struct srp_tsk_mgmt  { u8 _[24]; };
struct srp_rsp       { u8 _[36]; };

/* union srp_iu. modeled on viosrp.h. */
union srp_iu {
	struct srp_login_req login_req;
	struct srp_login_rsp login_rsp;
	struct srp_login_rej login_rej;
	struct srp_t_logout  t_logout;
	struct srp_tsk_mgmt  tsk_mgmt;
	struct srp_cmd       cmd;
	struct srp_rsp       rsp;
	u8 reserved[SRP_MAX_IU_LEN];
};

struct mad_iu_stub { u8 _[SRP_MAX_IU_LEN]; };
union viosrp_iu {
	union srp_iu srp;
	struct mad_iu_stub mad;
};

struct viosrp_crq { u64 IU_data_ptr; };
struct list_head { struct list_head *next, *prev; };

struct srp_event_struct {
	void *xfer_iu;
	void *cmnd;
	struct list_head list;
	void (*done)(struct srp_event_struct *);
	struct viosrp_crq crq;
	void *hostdata;
	int free;
	union viosrp_iu iu;
	void (*cmnd_done)(void *);
};

/* IOO bug function. copied from
   drivers/scsi/ibmvscsi/ibmvscsi.c */
static int ibmvscsi_queuecommand_lck_slice(struct srp_event_struct *evt_struct)
{
	struct srp_cmd *srp_cmd;

	/* Set up the actual SRP IU */
	srp_cmd = &evt_struct->iu.srp.cmd;
	/* buggy line, B */
	memset(srp_cmd, 0x00, SRP_MAX_IU_LEN);
	srp_cmd->opcode = SRP_CMD;
	
	return 0;
}
