
#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>

#include "macrotest.h"
#include "errtrace.h"

#include "strval.h"

#define S(str)	(str), (sizeof(str))

typedef struct str_val {
	uint64_t hash;
	size_t len;
	const char *d;
} str_val;

struct str_dict {
	size_t n, cap;
	struct {
		str_val key;
		const void *v;
	} entries[];
};

#define struct_field_size(s,f) (sizeof(((s){}).f))
static_assert(struct_field_size(str_dict, entries[0]) == sizeof(str_val)+sizeof(void*), "weird entry sizes");

#define entrysize struct_field_size(str_dict, entries[0])
#define str_dict_size(n) (sizeof(str_dict) + (n) * entrysize)

static_assert(sizeof(str_dict) == 2*sizeof(size_t), "weird dict sizes");
static_assert(str_dict_size(0) == sizeof(str_dict), "weird dict base size");
static_assert(str_dict_size(1) == str_dict_size(0) + sizeof(str_val) + sizeof(void *), "weird dict size delta");


static inline uint64_t o1hash(const void *key, size_t len) {
  const uint8_t *p=(const uint8_t*)key;
  if(len>=4) {
    uint32_t first, middle, last;
    memcpy(&first,p,4);
    memcpy(&middle,p+(len>>1)-2,4);
    memcpy(&last,p+len-4,4);
    return (uint64_t)(first+last)*middle;
  }
  if(len){
    uint64_t tail=((((uint32_t)p[0])<<16) | (((uint32_t)p[len>>1])<<8) | p[len-1]);
    return tail*0xa0761d6478bd642full;
  }
  return  0;
}


str_dict *new_dict(size_t startsize) {
	startsize = (startsize < 10) ? 10 : startsize;
	str_dict *d = malloc(str_dict_size(startsize));
	if (!d) return Tv(NULL);

	d->n = 0;
	d->cap = startsize;

	return d;
}


static str_dict *grow_dict(str_dict *d) {
	size_t newcap = (d->cap < 1 ) ? 1 : d->cap * 2;

	d = realloc(d, str_dict_size(newcap));
	if (!d) return Tv(NULL);

	d->cap = newcap;
	return d;
}

static str_val intern_str(const char *str, size_t len) {
	str_val sv = {
		.hash = o1hash(str, len),
		.len = len,
		.d = str,
	};

	return sv;
}

str_dict *add_str(str_dict *d, const char *str, size_t len, const void *val) {
	if (d->n >= d->cap) {
		d = grow_dict(d);
	}
	if (!d) return Tv(NULL);

	d->entries[d->n].key = intern_str(str, len);
	d->entries[d->n].v = val;
	d->n++;

	return d;
}


int strval_comp(const void *a, const void *b) {
	const str_val *sa = a, *sb = b;

	if (sa->hash > sb->hash)
		return 1;

	if (sa->hash < sb->hash)
		return -1;

	int r = strncmp(sa->d, sb->d, sa->len < sb->len ? sa->len : sb->len);
	if (r) return r;

	ssize_t szdiff = sa->len - sb->len;
	return szdiff > 0 ? 1 :
			szdiff < 0 ? -1 : 0;
}


str_dict *pack_dict(str_dict *d) {
	qsort(d->entries, d->n, entrysize, strval_comp);

	d = realloc(d, str_dict_size(d->n));
	if (!d) return Tv(NULL);

	d->cap = d->n;
	return d;
}

const void *query_str_dict(const str_dict *d, const char *str, size_t len) {
	str_val sv = intern_str(str, len);

	typeof(&d->entries[0]) p = bsearch(&sv, d->entries, d->n, entrysize, strval_comp);
	if (!p) return NULL;
	return p->v;
}



begin_test(strval)

	do_test("basic use") {
		str_dict *d = new_dict(0);
		assert(d);

		const void *p = query_str_dict(d, S("one"));
		assert(!p);

		d = add_str(d, S("one"), (void *)4);
		assert(d);

		d = pack_dict(d);
		assert(d);

		p = query_str_dict(d, S("one"));
		assert((uintptr_t)p == 4);

		d = pack_dict(d);
		assert(d);

		d = add_str(d, S("second"), (void *)20);
		assert(d);

		p = query_str_dict(d, S("second"));
		assert((uintptr_t)p == 20);

		p = query_str_dict(d, S("one"));
		assert((uintptr_t)p == 4);
	}

	do_test("many elements") {
		const int num_samples = 100,
			sample_size = 20;

		char samps[num_samples][sample_size];
		str_dict *d = new_dict(20);

		for (int i = 0; i < num_samples; i++) {
			char *s = samps[i];
			for (int k = 0; k < sample_size; k++) {
				s[k] = rand() % 256;
			}

			d = add_str(d, s, sample_size, (void*)(intptr_t)i);
		}

		assert(d->cap >= (unsigned)num_samples);
		d = pack_dict(d);
		assert(d->cap == (unsigned)num_samples);

		for (int i = 0; i < num_samples; i++) {
			const void *p = query_str_dict(d, samps[i], sample_size);
			assert((intptr_t)p == i);
		}
	}

end_test
