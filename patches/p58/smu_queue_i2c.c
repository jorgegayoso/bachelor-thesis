#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>

typedef uint8_t  u8;

#define SMU_I2C_READ_MAX	0x1d
#define SMU_I2C_WRITE_MAX	0x15

#define SMU_I2C_TRANSFER_SIMPLE   0
#define SMU_I2C_TRANSFER_STDSUB   1
#define SMU_I2C_TRANSFER_COMBINED 2

#define SMU_CMD_I2C_COMMAND       0x9a

#define ENODEV 19
#define EINVAL 22
#define DPRINTK(...)  ((void)0)
#define fallthrough   __attribute__((fallthrough))

struct list_head { struct list_head *next, *prev; };
struct smu_cmd {
	u8	cmd;
	int	data_len;
	int	reply_len;
	void	*reply_buf;
	void	*data_buf;
	int	status;
	void	(*done)(struct smu_cmd *, void *);
	void	*misc;
};

/* SMU i2c header, exactly matches i2c header on wire */
struct smu_i2c_param
{
	u8	bus;		/* SMU bus ID (from device tree) */
	u8	type;		/* i2c transfer type */
	u8	devaddr;	/* device address (includes direction) */
	u8	sublen;		/* subaddress length */
	u8	subaddr[3];	/* subaddress */
	u8	caddr;		/* combined address, filled by SMU driver */
	u8	datalen;	/* length of transfer */
	u8	data[SMU_I2C_READ_MAX];	/* data */
};

struct smu_i2c_cmd
{
	/* public */
	struct smu_i2c_param	info;
	void			(*done)(struct smu_i2c_cmd *cmd, void *misc);
	void			*misc;
	int			status; /* 1 = pending, 0 = ok, <0 = fail */

	/* private */
	struct smu_cmd		scmd;
	int			read;
	int			stage;
	int			retries;
	u8			pdata[32];
	struct list_head	link;
};

static int smu_dummy_state;
static int *smu = &smu_dummy_state;

static void smu_i2c_low_completion(struct smu_cmd *c, void *misc)
{ (void)c; (void)misc; }

/* IOO bug function. copied from
   drivers/macintosh/smu.c */
static int smu_queue_i2c(struct smu_i2c_cmd *cmd)
{
	unsigned long flags;
	(void)flags;

	if (smu == NULL)
		return -ENODEV;

	/* Fill most fields of scmd */
	cmd->scmd.cmd = SMU_CMD_I2C_COMMAND;
	cmd->scmd.done = smu_i2c_low_completion;
	cmd->scmd.misc = cmd;
	cmd->scmd.reply_buf = cmd->pdata;
	cmd->scmd.reply_len = sizeof(cmd->pdata);
	cmd->scmd.data_buf = (u8 *)(char *)&cmd->info;
	cmd->scmd.status = 1;
	cmd->stage = 0;
	cmd->pdata[0] = 0xff;
	cmd->retries = 20;
	cmd->status = 1;

	/* Check transfer type, sanitize some "info" fields
	 * based on transfer type and do more checking
	 */
	cmd->info.caddr = cmd->info.devaddr;
	cmd->read = cmd->info.devaddr & 0x01;
	switch(cmd->info.type) {
	case SMU_I2C_TRANSFER_SIMPLE:
	    /* buggy line, B3 */
		memset(&cmd->info.sublen, 0, 4);
		break;
	case SMU_I2C_TRANSFER_COMBINED:
		cmd->info.devaddr &= 0xfe;
		fallthrough;
	case SMU_I2C_TRANSFER_STDSUB:
		if (cmd->info.sublen > 3)
			return -EINVAL;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}
