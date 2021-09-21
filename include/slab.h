#ifndef SLAB_H
#define SLAB_H

#include <stddef.h>

struct slab {
	size_t block_size;
	size_t block_count;
	struct slab_item *free_list;
	struct slab_item *items;
	void *mem;
};

struct slab_item {
	struct slab_item *next;
};

void slab_init(struct slab *);
void *slab_alloc(struct slab *);
void slab_free(struct slab *, void *);

#endif // SLAB_H
