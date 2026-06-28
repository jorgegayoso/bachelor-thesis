#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <errno.h>

typedef uint32_t u32;

#define likely(x)  (x)
#define BUG() do { abort(); } while (0)
#define ERR_PTR(e) ((void *)(intptr_t)(e))

#define XFRM_POLICY_TYPE_MAX 2
#define AF_INET   2
#define AF_INET6  10
#define DST_OBSOLETE_NONE 0

struct dst_entry { unsigned char _opaque[112]; };
struct rtable    { unsigned char _opaque[112]; };
struct rt6_info  { unsigned char _opaque[112]; };

struct net               { int dummy; };
struct net_device        { int dummy; };
struct flowi             { int dummy; };
struct xfrm_policy       { int dummy; };
struct dst_ops           { int dummy; };
struct xfrm_policy_afinfo { struct dst_ops *dst_ops; };

struct xfrm_dst {
	union {
		struct dst_entry	dst;
		struct rtable		rt;
		struct rt6_info		rt6;
	} u;
	struct dst_entry *route;
	struct dst_entry *child;
	struct dst_entry *path;
	struct xfrm_policy *pols[XFRM_POLICY_TYPE_MAX];
	int num_pols, num_xfrms;
	u32 xfrm_genid;
	u32 policy_genid;
	u32 route_mtu_cached;
	u32 child_mtu_cached;
	u32 route_cookie;
	u32 path_cookie;
};

static struct xfrm_dst g_xdst_storage;
static struct xfrm_dst captured;

static const struct xfrm_policy_afinfo dummy_afinfo = { .dst_ops = NULL };
static const struct xfrm_policy_afinfo *xfrm_policy_get_afinfo(int family)
{ (void)family; return &dummy_afinfo; }

static struct xfrm_dst *dst_alloc(struct dst_ops *ops, void *dev,
                                  int initial_ref, int obsolete, int flags)
{
	(void)ops; (void)dev; (void)initial_ref; (void)obsolete; (void)flags;
	memset(&g_xdst_storage, 0xAA, sizeof(g_xdst_storage));
	return &g_xdst_storage;
}

#define rcu_read_unlock() ((void)0)

/* IOO bug function. copied from
   net/xfrm/xfrm_policy.c */
static inline struct xfrm_dst *xfrm_alloc_dst(struct net *net, int family)
{
	const struct xfrm_policy_afinfo *afinfo = xfrm_policy_get_afinfo(family);
	struct dst_ops *dst_ops;
	struct xfrm_dst *xdst;

	if (!afinfo)
		return ERR_PTR(-EINVAL);

	switch (family) {
	case AF_INET:
		dst_ops = afinfo->dst_ops;
		break;
#if 0 /* CONFIG_IPV6 */
	case AF_INET6:
		dst_ops = afinfo->dst_ops;
		break;
#endif
	default:
		BUG();
	}
	xdst = dst_alloc(dst_ops, NULL, 1, DST_OBSOLETE_NONE, 0);

	if (likely(xdst)) {
		struct dst_entry *dst = &xdst->u.dst;

		/* bugg line, B2 */
		memset(dst + 1, 0, sizeof(*xdst) - sizeof(*dst));
	} else
		xdst = ERR_PTR(-ENOBUFS);

	rcu_read_unlock();

	if (xdst) captured = *xdst;
	return xdst;
}
