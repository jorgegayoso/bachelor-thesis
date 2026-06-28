#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>

#define MAX_ATR        33
#define CM4000_MAX_DEV 4

struct pcmcia_device { int dummy; };

typedef struct { unsigned char _opaque[24]; } wait_queue_head_t;

struct timer_list { unsigned char _opaque[40]; };

struct cm4000_dev {
	struct pcmcia_device *p_dev;

	unsigned char atr[MAX_ATR];
	unsigned char rbuf[512];
	unsigned char sbuf[512];

	wait_queue_head_t devq;		/* when removing cardman must not be
					   zeroed! */

	wait_queue_head_t ioq;		/* if IO is locked, wait on this Q */
	wait_queue_head_t atrq;		/* wait for ATR valid */
	wait_queue_head_t readq;	/* used by write to wake blk.read */

	/* warning: do not move this fields.
	 * initialising to zero depends on it - see ZERO_DEV below.  */
	unsigned char atr_csum;
	unsigned char atr_len_retry;
	unsigned short atr_len;
	unsigned short rlen;	/* bytes avail. after write */
	unsigned short rpos;	/* latest read pos. write zeroes */
	unsigned char procbyte;	/* T=0 procedure byte */
	unsigned char mstate;	/* state of card monitor */
	unsigned char cwarn;	/* slow down warning */
	unsigned char flags0;	/* cardman IO-flags 0 */
	unsigned char flags1;	/* cardman IO-flags 1 */
	unsigned int mdelay;	/* variable monitor speeds, in jiffies */

	unsigned int baudv;	/* baud value for speed */
	unsigned char ta1;
	unsigned char proto;	/* T=0, T=1, ... */
	unsigned long flags;	/* lock+flags (MONITOR,IO,ATR) * for concurrent
				   access */

	unsigned char pts[4];

	struct timer_list timer;	/* used to keep monitor running */
	int monitor_running;
};

/* IOO bug def. copied from
   drivers/char/pcmcia/cm4000_cs.c */
/* bugg line, B1 */
#define	ZERO_DEV(dev)  						\
	memset(&dev->atr_csum,0,				\
		sizeof(struct cm4000_dev) - 			\
		offsetof(struct cm4000_dev, atr_csum))

static void cmm_open_slice(struct cm4000_dev *dev)
{
	ZERO_DEV(dev);
}
