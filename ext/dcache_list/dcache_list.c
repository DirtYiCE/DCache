#include <ruby.h>
#include <time.h>

static ID CMP;

typedef struct ItemStruct Item;

/**
 * An item in the linked list
 */
struct ItemStruct
{
	VALUE key;   /*< This item's key */
	VALUE value; /*< Item's value */
	time_t time; /*< Time when this item will time out. */
	Item * prev; /*< Previous item in the linked list, or NULL if it's the head */
	Item * next; /*< Next item in the linked list, or NULL if it's the last */
};

/**
 * Basic structure used by DCacheList
 */
typedef struct
{
	Item * head; /*< Head of the linked list */
	Item * end;  /*< End of the linked list */

	/**
	 * Statistical variables
	 */
	unsigned long long int hits, misses, stored, removed;
	size_t length;     /*< Number of stored items / linked list's length */
	size_t max_items;  /*< Maximum number of items that can be stored. (size_t)
						*  -1 means no limit */
	time_t last_clean; /*< Time when last clean operation ran. */
} Struct;

/**
 * Gets the associated structure.
 * @param[in] self self
 * @param[out] var name of the variable that'll contain the pointer to the
 *   structure.
 */
#define GET_STRUCT(self, var)					\
	Struct * var;								\
	Data_Get_Struct(self, Struct, var);

/**
 * Removes an item from cache.
 * @param[in] s struct associated with self.
 * @param[in] i remove this item.
 */
static Item * item_remove(Struct * s, Item * i)
{
	/* debug, uncomment to enable :p */
	/* printf("removing %zd (p: %zd, n: %zd)\n", (size_t)i, (size_t)i->prev, */
	/* 	   (size_t)i->next); */
	/* if (i->prev) printf("prev's next: %zd\n", (size_t)i->prev->next); */
	/* if (i->next) printf("next's prev: %zd\n", (size_t)i->next->prev); */

	if (i->prev)
		i->prev->next = i->next;
	else
		s->head = i->next;

	if (i->next)
		i->next->prev = i->prev;
	else
		s->end = i->prev;

	Item * ret = i->next;
	free(i);
	--s->length;
	++s->removed;
	return ret;
}

/**
 * Mark elements as used for Ruby's GC.
 * @param[in] s struct associated with self.
 */
static void struct_mark(Struct * s)
{
	time_t tim = time(NULL);
	Item * i;

	for (i = s->head; i; )
	{
		if (i->time > tim || i->time == 0)
		{
			rb_gc_mark(i->key);
			rb_gc_mark(i->value);
			i = i->next;
		}
		else
		{
			i = item_remove(s, i);
		}
	}
	s->last_clean = tim;
}

/**
 * Clears outdated items.
 * @param[in] s struct associated with self.
 */
static void list_clean(Struct * s)
{
	time_t tim = time(NULL);
	if (s->last_clean == tim)
		return;
	Item * i;

	for (i = s->head; i; )
	{
		if (i->time > tim || i->time == 0)
		{
			i = i->next;
		}
		else
			i = item_remove(s, i);
	}
	s->last_clean = tim;
}

/**
 * Deletes list from a starting point, Doesn't update <tt>s->head</tt> and
 * <tt>s->end</tt>
 * @param[in] s struct associated with self.
 * @param[in] head start freeing from this item
 */
static void list_free(Struct * s, Item * head)
{
	Item * i;
	for (i = head; i; )
	{
		Item * n = i->next;
		++s->removed;
		free(i);
		i = n;
	}
}

/**
 * Frees every memory associated with this object.
 * @param[in] s struct associated with self.
 */
static void struct_free(Struct * s)
{
	list_free(s, s->head);
	free(s);
}

/**
 * Moves the selected Item to the begin of the list chain.
 * @param[in] s struct associated with self.
 * @param[in] i: the item to move
 */
static void item_bring_front(Struct * s, Item * i)
{
	if (!i->prev) return;

	i->prev->next = i->next;
	if (i->next)
		i->next->prev = i->prev;
	else
		s->end = i->prev;

	s->head->prev = i;
	i->next = s->head;
	s->head = i;
	i->prev = NULL;
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
 * @return [Number] number of items that has been stored up to now.
 */
static VALUE method_get_stored(VALUE self)
{
	GET_STRUCT(self, s);
	return INT2NUM(s->stored);
}

/*
 * @return [Number] number of items that has been removed from cache until now.
 */
static VALUE method_get_removed(VALUE self)
{
	GET_STRUCT(self, s);
	return INT2NUM(s->removed);
}

/*
 * @return [Number, nil] maximum allowed number of stored items, or +nil+ if
 *   there's no such limit,
 */
static VALUE method_get_max_items(VALUE self)
{
	GET_STRUCT(self, s);
	if (s->max_items == (size_t) -1)
		return Qnil;
	else
		return INT2NUM(s->max_items);
}

/* @overload max_items= (max_item)
 *  Set the maximum allowed number of stored items. If the new limit is smaller
 *  than the number of already stored elements, the list will automatically
 *  truncated.
 *  @param [Number, nil] max_items maximum allowed number of stored items, or
 *    +nil+ if there's no such limit,
 *  @return [Number, nil] *max_items*
 */
static VALUE method_set_max_items(VALUE self, VALUE max_items)
{
	GET_STRUCT(self, s);
	if (max_items == Qnil)
		s->max_items = -1;
	else
		s->max_items = NUM2ULONG(max_items);

	if (s->length > s->max_items)
		list_clean(s);
	if (s->length > s->max_items)
	{
		int i = 0;
		Item * it = s->head;
		for (; i < s->max_items; it = it->next, ++i);
		it->prev->next = NULL;
		s->end = it->prev;
		list_free(s, it);
		s->length = s->max_items;
	}

	return method_get_max_items(self);
}

/*
 * @return [Number] number of cached elements.
 */
static VALUE method_get_length(VALUE self)
{
	GET_STRUCT(self, s);
	return INT2NUM(s->length);
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

	Item * i;
	for (i = s->head; i; )
	{
		if (rb_funcall(i->key, CMP, 1, key) == Qtrue)
		{
			item_bring_front(s, i);
			i->value = val;
			if (timeout == Qnil)
				i->time = 0;
			else
				i->time = time(NULL) + NUM2ULONG(timeout);
			return val;
		}
		i = i->next;
	}

	if (s->length >= s->max_items)
		list_clean(s);
	if (s->length >= s->max_items)
		item_remove(s, s->end);

	i = ALLOC(Item);
	i->key = key;
	i->value = val;
	if (timeout == Qnil)
		i->time = 0;
	else
		i->time = time(NULL) + NUM2ULONG(timeout);
	i->next = s->head;
	if (s->head)
		s->head->prev = i;
	else
		s->end = i;
	i->prev = NULL;
	s->head = i;
	++s->stored;
	++s->length;

	return val;
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
	Item * i;
	for (i = s->head; i; i = i->next)
	{
		if (i->time <= tim && i->time != 0) continue;
		if (rb_funcall(i->key, CMP, 1, key) == Qtrue)
		{
			++s->hits;
			item_bring_front(s, i);
			return i->value;
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
static VALUE method_remove(int argc, VALUE * argv, VALUE self)
{
	GET_STRUCT(self, s);
	VALUE todel;
	rb_scan_args(argc, argv, "01", &todel);
	time_t tim = time(NULL);
	int has_block = rb_block_given_p();

	Item * i;
	for (i = s->head; i; )
	{
		VALUE to_del;
		if (i->time <= tim && i->time != 0) to_del = Qtrue;
		else if (has_block)
		{
			VALUE ary = rb_ary_new3(2, i->key, i->value);
			to_del = rb_yield(ary);
		}
		else
			to_del = rb_funcall(i->key, CMP, 1, todel);

		if (to_del != Qfalse && to_del != Qnil)
			i = item_remove(s, i);
		else
			i = i->next;
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
	list_free(s, s->head);
	s->head = s->end = NULL;
	s->length = 0;

	return Qnil;
}

/* @overload new(max_items = nil)
 *  Creates a new list based DCache backend.
 *  @param [Number, nil] max_items maximum number of stored items, +nil+ if
 *    there's no such limit.
 *  @return [DCacheList] a new object.
 */
static VALUE method_new(int argc, VALUE * argv, VALUE class)
{
	VALUE max_items;
	rb_scan_args(argc, argv, "01", &max_items);

	Struct * s;
	s = ALLOC(Struct);
	s->head = s->end = NULL;
	s->hits = s->misses = 0;
	s->stored = s->removed = 0;
	if (max_items == Qnil)
		s->max_items = -1;
	else
		s->max_items = NUM2ULONG(max_items);
	s->length = 0;
	s->last_clean = 0;

	VALUE x = Data_Wrap_Struct(class, struct_mark, struct_free, s);
	rb_obj_call_init(x, 0, NULL);

	return x;
}

void Init_dcache_list()
{
	CMP = rb_intern("eql?");

	VALUE dcache = rb_define_class("DCacheList", rb_cObject);
	rb_define_singleton_method(dcache, "new", method_new, -1);
	rb_define_method(dcache, "add", method_add, 3);
	rb_define_method(dcache, "get", method_get, -1);
	rb_define_method(dcache, "remove", method_remove, -1);
	rb_define_method(dcache, "clear", method_clear, 0);

	rb_define_method(dcache, "hits", method_get_hits, 0);
	rb_define_method(dcache, "misses", method_get_misses, 0);
	rb_define_method(dcache, "stored", method_get_stored, 0);
	rb_define_method(dcache, "removed", method_get_removed, 0);
	rb_define_method(dcache, "max_items", method_get_max_items, 0);
	rb_define_method(dcache, "max_items=", method_set_max_items, 1);
	rb_define_method(dcache, "length", method_get_length, 0);
}
