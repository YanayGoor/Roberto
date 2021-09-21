#include <malloc.h>
#include <slab.h>

#define ITEM_GET_INDEX(item) ((item)-slab->items)
#define MEM_GET_INDEX(item)	 (((item)-slab->mem) / slab->block_size)

#define ITEM_BY_INDEX(index) (slab->items + index)
#define MEM_BY_INDEX(index)	 (slab->mem + (index * slab->block_size))

static inline struct slab_item *list_pop(struct slab_item **head) {
	struct slab_item *first = *head;
	*head = (*head)->next;
	return first;
}

static inline void list_insert(struct slab_item **head,
							   struct slab_item *item) {
	item->next = *head;
	*head = item;
}

void slab_init(struct slab *slab) {
	slab->items = malloc(slab->block_count * sizeof(struct slab_item));
	slab->mem = malloc(slab->block_count * slab->block_size);

	for (int i = 0; i < slab->block_count - 1; i++) {
		slab->items[i].next = slab->items + i + 1;
	}
	slab->items[slab->block_count].next = NULL;

	slab->free_list = slab->items;
}

void *slab_alloc(struct slab *slab) {
	struct slab_item *item = list_pop(&slab->free_list);
	return MEM_BY_INDEX(ITEM_GET_INDEX(item));
}

void slab_free(struct slab *slab, void *allocated) {
	struct slab_item *item = ITEM_BY_INDEX(MEM_GET_INDEX(allocated));
	list_insert(&slab->free_list, item);
}
