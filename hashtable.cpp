#include <cmath>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <endian.h>
#include <stdlib.h>
#include <assert.h>

#include "hashtable.h"

static void ht_init(HTab *htab, size_t n) {
	assert(n > 0 && (((n-1) & n) == 0)); // n > 0 and power of 2
	htab->mask = n - 1;
	htab->size = 0;
	htab->buckets = (HNode**) calloc(n, sizeof(HNode*));
}

static void ht_insert_bucket(HTab *htab, HNode *hnode) {
	
	/*
	// check the position inside the array
		hnode->next = NULL;
		size_t pos = hnode->hcode % htab->mask; 

		// get the beggining of the linked list
		HNode *bucket_root = htab->buckets[pos];

		// empty bucket case
		if (!bucket_root) {
			htab->buckets[pos] = hnode;
			return;
		}
		
		while (bucket_root->next != NULL) {
			bucket_root = bucket_root->next;
		}
		bucket_root->next = hnode;
	*/

	// that could work, but for efficiency we will insert at the beggining
	// so that we have O(1) insertion
	
	size_t pos = hnode->hcode & htab->mask;
	HNode *next =  htab->buckets[pos];
	hnode->next = next;
	htab->buckets[pos] = hnode; // redirect it to point to this element
	++htab->size;

}

// function used for deletion
static HNode **ht_lookup(HTab *htab, HNode *key, bool (*eq)(HNode *, HNode*)) {
	fprintf(stderr, "should not get past this point1\n");
	if (!htab->buckets) {
		return NULL; // should use nullptr realistically, but hey it is what it is	
	}
	fprintf(stderr, "should not get past this point\n");
	
	// find the corresponding bucket
	size_t pos = key->hcode & htab->mask;

	// go through the whole bucket
	// return the address of the parent 
	HNode **parent= &htab->buckets[pos];
	for (HNode *cur; (cur = *parent) != NULL; parent= &cur->next) {
		if (eq(cur, key) && cur->hcode == key->hcode)
			return parent;
	}
	return NULL;
}

// we use the function ht_lookup before hand
static HNode *ht_deletion(HTab *htab, HNode **parent) {
	// i do not actually free the said memory
	HNode *node = *parent;
	*parent =	node->next;
	--htab->size;
	return node;
}

static void hmap_trigger_rehash(HMap *hmap) {
		// move hmap->newer to hmap->older
		// set migrate_pos to 0, since we didn't migrate anything, yet
		// simply shallow copy the pointer, we want same location	so we don't waste time
		hmap->older = hmap->newer;
		// for the new size, shift the bits from the mask to the left by 1
		// since mask is already a power of 2 - 1, it should work just fine 
		// god forbid it goes beyond 32 bits we're are f'ed brothers
		ht_init(&hmap->newer, ((hmap->newer.mask + 1) << 1));
		hmap->migrate_pos = 0;
}

static void hm_help_rehash(HMap *hmap) {
	size_t nwork = 0;	
	while (nwork < rehash_work && hmap->older.size > 0) {
		HNode **node = &hmap->older.buckets[hmap->migrate_pos];
		if (!*node) {
			++hmap->migrate_pos;
			continue;
		}
		ht_insert_bucket(&hmap->newer, ht_deletion(&hmap->older, node));
		++nwork;
	}

	if (!hmap->older.size && hmap->older.buckets) {
		free(hmap->older.buckets);
		hmap->older = HTab{};
	}
}

HNode *hmap_lookup(HMap *hmap, HNode *node, bool (*eq)(HNode*, HNode*)) {
	HNode ** from = ht_lookup(&hmap->newer, node, eq); 
	fprintf(stderr, "length: %ld\n", hmap->newer.size);

	if (!from)
		from = ht_lookup(&hmap->older, node, eq);
	
	if (from) {
		return *from;	
	}

	return NULL;
}

HNode *hmap_deletion(HMap *hmap, HNode *node, bool (*eq)(HNode*, HNode*)) {
	if (!hmap->newer.buckets)
		ht_init(&hmap->newer, 4);
	HNode **result = ht_lookup(&hmap->newer, node, eq);
	if (result)
		return ht_deletion(&hmap->newer, result);
	
	result = ht_lookup(&hmap->older, node, eq);
	if (result)
		return ht_deletion(&hmap->older, result);

	return NULL;
}

void hmap_insert(HMap *hmap, HNode *node) {
	if (!hmap->newer.buckets)	
		ht_init(&hmap->newer, 4); // start with a base size of 4
	
	ht_insert_bucket(&hmap->newer, node);

	// check if we need to move some things cause we are rehashing
	// moving houses type shi
	if (!hmap->older.buckets) {
		size_t treshhold = (hmap->newer.mask + 1) * max_load_factor;
		if (treshhold * 1.0 / hmap->newer.size >= 0.75)
			hmap_trigger_rehash(hmap);
	}
	hm_help_rehash(hmap);
}

