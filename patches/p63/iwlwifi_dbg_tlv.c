#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>

typedef uint8_t  u8;
typedef uint32_t u32;
typedef uint32_t __le32;

#define le32_to_cpu(x) ((u32)(x))
#define cpu_to_le32(x) ((__le32)(x))
#define ENOMEM 12
#define GFP_KERNEL 0

struct list_head { struct list_head *next, *prev; };

static void INIT_LIST_HEAD(struct list_head *h) { h->next = h->prev = h; }
static void list_add_tail(struct list_head *new, struct list_head *head)
{
	new->next = head;
	new->prev = head->prev;
	head->prev->next = new;
	head->prev = new;
}

struct iwl_ucode_tlv {
	__le32 type;		/* see above */
	__le32 length;		/* not including type/length fields */
	u8 data[0];
};

struct iwl_dbg_tlv_node {
	struct list_head list;
	struct iwl_ucode_tlv tlv;
};

static void *kzalloc(size_t sz, int flags) { (void)flags; return calloc(1, sz); }

static struct iwl_dbg_tlv_node *captured_node;
static size_t captured_alloc_sz;

/* IOO bug function. copied from
   drivers/net/wireless/intel/iwlwifi/iwl-dbg-tlv.c */
static int iwl_dbg_tlv_add(struct iwl_ucode_tlv *tlv, struct list_head *list)
{
	u32 len = le32_to_cpu(tlv->length);
	struct iwl_dbg_tlv_node *node;

	node = kzalloc(sizeof(*node) + len, GFP_KERNEL);
	if (!node)
		return -ENOMEM;

	/* buggy line, A3 */
	memcpy(&node->tlv, tlv, sizeof(node->tlv) + len);
	list_add_tail(&node->list, list);

	captured_node     = node;
	captured_alloc_sz = sizeof(*node) + len;
	return 0;
}
