#include <ruby.h>
#include <time.h>

static ID CMP;

typedef struct
{
	VALUE key;
	VALUE value;
	time_t time;
} SubStruct;

typedef struct
{
	unsigned int head;
	unsigned int items;
	unsigned long long int hits;
	unsigned long long int misses;
	SubStruct s[];
} Struct;

/**
 * Mark elements in cache for Ruby's GC.
 * @param[in] s struct associated with self.
 */
static void struct_mark(Struct * s)
{
	time_t tim = time(NULL);
	int i;

	for(i = 0; i < s->items; ++i)
	{
		SubStruct * it = &(s->s[i]);

		if (it->time > tim || it->time == 0)
		{
			rb_gc_mark(it->key);
			rb_gc_mark(it->value);
		}
	}
}

/**
 * Gets the associated structure.
 * @param[in] self self
 * @param[out] var name of the variable that'll contain the pointer to the
 *   structure.
 */
#define GET_STRUCT(self, var)					\
	Struct * var;								\
	Data_Get_Struct(self, Struct, var);

/*
 * @return [Number] maximum number of items cache can store.
 */
static VALUE method_get_max_items(VALUE self)
{
	GET_STRUCT(self, s);
	return INT2NUM(s->items);
}

/*
 * @return [Number] number of cache hits.
 */
static VALUE method_get_hits(VALUE self)
{
	GET_STRUCT(self, s);
	return INT2NUM(s->hits);
}

/*
 * @return [Number] number of cache misses.
 */
static VALUE method_get_misses(VALUE self)
{
	GET_STRUCT(self, s);
	return INT2NUM(s->misses);
}

/*
 * @return [Number] number of currently cached items.
 */
static VALUE method_get_stored_items(VALUE self)
{
	GET_STRUCT(self, s);
	time_t tim = time(NULL);
	int cnt = 0;

	int i;
	for (i = 0; i < s->items; ++i)
		if(s->s[i].time > tim || s->s[i].time == 0)
			++cnt;

	return INT2NUM(cnt);
}

/* @overload add(key, value, timeout)
 *  Add a new value to cache.
 *  @param [#eql?] key an unique identifier (in the cache)
 *  @param value the value
 *  @param [Number, nil] timeout time in seconds after the value will be purged
 *    from cache. +nil+ if there's no such limit.
 *  @return *value*
 */
static VALUE method_add(VALUE self, VALUE key, VALUE val, VALUE timeout)
{
	GET_STRUCT(self, s);
	if (++s->head >= s->items)
		s->head = 0;

	SubStruct * it = &(s->s[s->head]);
	it->key = key;
	it->value = val;
	if (timeout == Qnil)
		it->time = 0;
	else
		it->time = time(NULL) + NUM2INT(timeout);

	return val;
}

/**
 * Decreases the item pointer, wrapping around the start if needed.
 * @param[in,out] n pointer to the current pointer, will be changed directly.
 * @param[in] cnt size of the cache
 */
static void dec(int * n, int cnt)
{
	--(*n);
	if (*n < 0) *n = cnt - 1;
}

/*
 * Tries to get a value from the cache.
 * @overload get(key, default = nil)
 *   Tries to get a value from cache. If it fails, returns a default value.
 *   @param [#eql?] key the key of the value to get
 *   @param default default return value.
 *   @return the value from cache if found, *default* otherwise
 * @overload get(key)
 *   Tries to get a value from cache. If it fails, executes the block.
 *   @param [#eql?] key the key of the value to get
 *   @yield if key is not found.
 *   @return the value from cache if found, or yielded block's return value.
 */
static VALUE method_get(int argc, VALUE * argv, VALUE self)
{
	GET_STRUCT(self, s);
	VALUE key, default_val;
	rb_scan_args(argc, argv, "11", &key, &default_val);

	time_t tim = time(NULL);
	int end = s->head + 1;
	if (end >= s->items) end = 0;

	int i;
	for (i = s->head; i != end; dec(&i, s->items))
	{
		SubStruct * it = &(s->s[i]);
		if (it->time <= tim && it->time != 0) continue;
		if (rb_funcall(it->key, CMP, 1, key) == Qtrue)
		{
			++s->hits;
			return it->value;
		}
	}

	++s->misses;
	if (rb_block_given_p())
	{
		return rb_yield(Qnil);
	}

	return default_val;
}

/*
 * Removes a given key or keys from the cache.
 * @overload remove(key)
 *   @param key remove this key from the cache
 * @overload remove() { |key, value| ... }
 *   @yield [key, value] for each item in the cache,
 *   @yieldparam key key of the current item.
 *   @yieldparam value value associated with the *key*.
 *   @yieldreturn [Boolean] +true+ if you want to delete this item
 * @return [nil]
 */
static VALUE mehod_remove(int argc, VALUE * argv, VALUE self)
{
	GET_STRUCT(self, s);
	VALUE todel;
	rb_scan_args(argc, argv, "01", &todel);
	time_t tim = time(NULL);

	int has_block = rb_block_given_p();

	int i;
	for (i = 0; i < s->items; ++i)
	{
		SubStruct * it = &(s->s[i]);
		if (it->time > tim || it->time == 0)
		{
			VALUE to_del;
			if (has_block)
			{
				VALUE ary = rb_ary_new3(2, it->key, it->value);
				to_del = rb_yield(ary);
			}
			else
				to_del = rb_funcall(it->key, CMP, 1, todel);

			if (to_del != Qfalse && to_del != Qnil)
				it->time = 1;
		}
	}

	return Qnil;
}

/*
 * Clears the cache.
 * @return [nil]
 */
static VALUE method_clear(VALUE self)
{
	GET_STRUCT(self, s);

	int i;
	for (i = 0; i < s->items; ++i)
		s->s[i].time = 1;

	return Qnil;
}

/* @overload new(max_items)
 *  Creates a new array-based DCache backend with fixed size.
 *  @param [Number] max_items number of cache items this cache can store.
 *  @return [DCacheAry] the new object.
 */
static VALUE method_new(VALUE class, VALUE max_items)
{
	Struct * s;
	int items = NUM2INT(max_items);
	if (items < 1)
		rb_raise(rb_eArgError, "max_items is %d, it should >= 1", items);

	s = ruby_xmalloc(sizeof(Struct) + items * sizeof(SubStruct));
	s->head = 0;
	s->items = items;
	s->hits = s->misses = 0;

	int i;
	for (i = 0; i < items; ++i)
		s->s[i].time = 1;

	VALUE x = Data_Wrap_Struct(class, struct_mark, free, s);
	rb_obj_call_init(x, 0, NULL);

	return x;
}

void Init_dcache_ary()
{
	CMP = rb_intern("eql?");
	VALUE dcache = rb_define_class("DCacheAry", rb_cObject);
	rb_define_singleton_method(dcache, "new", method_new, 1);
	rb_define_method(dcache, "add", method_add, 3);
	rb_define_method(dcache, "get", method_get, -1);
	rb_define_method(dcache, "remove", mehod_remove, -1);
	rb_define_method(dcache, "clear", method_clear, 0);

	rb_define_method(dcache, "hits", method_get_hits, 0);
	rb_define_method(dcache, "misses", method_get_misses, 0);
	rb_define_method(dcache, "max_items", method_get_max_items, 0);
	rb_define_method(dcache, "length", method_get_stored_items, 0);
}
