#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>

typedef uint32_t u32;
typedef uint32_t __be32;

#define __packed __attribute__((packed))
#define cpu_to_be32(x) ((__be32)(x))

#define SHARED_SECRET_MAX 64

enum drbd_packet { P_SYNC_PARAM = 1, P_SYNC_PARAM89 = 2 };

#define DRBD_RESYNC_RATE_DEF      250
#define DRBD_C_PLAN_AHEAD_DEF      20
#define DRBD_C_DELAY_TARGET_DEF   100
#define DRBD_C_FILL_TARGET_DEF    100
#define DRBD_C_MAX_RATE_DEF      4096

struct p_rs_param_95 {
	u32 resync_rate;
	char verify_alg[SHARED_SECRET_MAX];
	char csums_alg[SHARED_SECRET_MAX];
	u32 c_plan_ahead;
	u32 c_delay_target;
	u32 c_fill_target;
	u32 c_max_rate;
} __packed;

struct net_conf  { char verify_alg[SHARED_SECRET_MAX]; char csums_alg[SHARED_SECRET_MAX]; };
struct disk_conf { u32 resync_rate, c_plan_ahead, c_delay_target,
                       c_fill_target, c_max_rate; };
struct drbd_device     { struct disk_conf *disk_conf_; int has_ldev; };
struct drbd_socket     { int dummy; };
struct drbd_connection { int agreed_pro_version; struct net_conf *net_conf;
                         struct drbd_socket data; };
struct drbd_peer_device { struct drbd_connection *connection;
                          struct drbd_device *device; };

static struct p_rs_param_95 g_packet;
static struct p_rs_param_95 captured;

static struct p_rs_param_95 *drbd_prepare_command(struct drbd_peer_device *pd,
                                                  struct drbd_socket *sock)
{ (void)pd; (void)sock; return &g_packet; }
static int drbd_send_command(struct drbd_peer_device *pd,
                             struct drbd_socket *sock, enum drbd_packet cmd,
                             int size, void *data, int data_size)
{ (void)pd;(void)sock;(void)cmd;(void)size;(void)data;(void)data_size; return 0; }
static int  get_ldev(struct drbd_device *d) { return d->has_ldev; }
static void put_ldev(struct drbd_device *d) { (void)d; }

#define rcu_read_lock()       ((void)0)
#define rcu_read_unlock()     ((void)0)
#define rcu_dereference(p)    (p)

/* IOO bug function. copied from
   drivers/block/drbd/drbd_main.c */
static int drbd_send_sync_param(struct drbd_peer_device *peer_device)
{
	struct drbd_socket *sock;
	struct p_rs_param_95 *p;
	int size;
	const int apv = peer_device->connection->agreed_pro_version;
	enum drbd_packet cmd;
	struct net_conf *nc;
	struct disk_conf *dc;

	sock = &peer_device->connection->data;
	p = drbd_prepare_command(peer_device, sock);
	if (!p)
		return -1;

	rcu_read_lock();
	nc = rcu_dereference(peer_device->connection->net_conf);

	size = /* apv >= 95 */ sizeof(struct p_rs_param_95);
	(void)apv;

	cmd = apv >= 89 ? P_SYNC_PARAM89 : P_SYNC_PARAM;

	/* initialize verify_alg and csums_alg */
	/* buggy line, B1 */
	memset(p->verify_alg, 0, 2 * SHARED_SECRET_MAX);

	if (get_ldev(peer_device->device)) {
		dc = rcu_dereference(peer_device->device->disk_conf_);
		p->resync_rate = cpu_to_be32(dc->resync_rate);
		p->c_plan_ahead = cpu_to_be32(dc->c_plan_ahead);
		p->c_delay_target = cpu_to_be32(dc->c_delay_target);
		p->c_fill_target = cpu_to_be32(dc->c_fill_target);
		p->c_max_rate = cpu_to_be32(dc->c_max_rate);
		put_ldev(peer_device->device);
	} else {
		p->resync_rate = cpu_to_be32(DRBD_RESYNC_RATE_DEF);
		p->c_plan_ahead = cpu_to_be32(DRBD_C_PLAN_AHEAD_DEF);
		p->c_delay_target = cpu_to_be32(DRBD_C_DELAY_TARGET_DEF);
		p->c_fill_target = cpu_to_be32(DRBD_C_FILL_TARGET_DEF);
		p->c_max_rate = cpu_to_be32(DRBD_C_MAX_RATE_DEF);
	}

	captured = *p;

	if (apv >= 88)
		strcpy(p->verify_alg, nc->verify_alg);
	if (apv >= 89)
		strcpy(p->csums_alg, nc->csums_alg);
	rcu_read_unlock();

	return drbd_send_command(peer_device, sock, cmd, size, NULL, 0);
}
