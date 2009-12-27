class DCache
  VERSION = '0.0.0'

  protected
  def self.embedded_reader *args
    args.flatten.each do |v|
      class_eval <<EOS
        def #{v}
          @backend.#{v}
        end
EOS
    end
  end
  public

  # Which DCache backend is used (can be +:list+ or +:ary+ currently)
  attr_reader :backend

  # Number of cache hits
  embedded_reader :hits
  # Number of cache misses
  embedded_reader :misses
  # Number of elements stored (only if <tt>backend == :list</tt>)
  embedded_reader :stored
  alias :added :stored
  # Number of elements removed from cache (only if <tt>backend == :list</tt>)
  embedded_reader :removed
  alias :deleted :removed
  # Number of elements currently stored in cache
  embedded_reader :length
  alias :size :length
  # Maximum number of elements that can be stored.
  embedded_reader :max_items
  # Set maximum number of elements.  Only if <tt>backend == :list</tt>.
  # * +val+: new maximum number of elements. If it's smaller than the amount of
  #   currently stored elements, old elements will be eliminated. If it's +nil+,
  #   there's no maximum.
  def max_items= val
    @backend.max_items = val
  end

  # Default timeout for new values
  attr_accessor :timeout

  # Initializes DCache.
  # * +backend+: which backend to use. Currently can be +:ary+ or +:list+.
  #   +:ary+::  is based on a fixed size array, and implements something like
  #             "Least Recently Added".
  #   +:list+:: uses a linked list to store the elements, so it's maximum size
  #             can be changed during runtime, and it implements a simple LRU
  #             cache.
  def initialize backend = :list, *args
    require "dcache_#{backend}"
    @backend = Kernel.const_get("DCache#{backend.capitalize}").new(*args)
    @timeout = nil
  end

  # Tries to get a value from cache. If it fails, executes the block if given, or
  # returns default
  # * +key+: the key of the value to get
  # * +default: return value if no block given and cache doesn't contain key.
  def get key, default = nil
    if block_given?
      @backend.get(key) { yield }
    else
      @backend.get key, default
    end
  end

  # Tries to get a value. If it fails, add it, then return.
  # * +key+: the key of the value to get
  # * +value+: value to set if get fails. If a block is passed, it'll be
  #   executed instead of this
  # * +timeout+: timeout of the newly added item
  def get_or_add key, value = nil, timeout = @timeout
    @backend.get key do
      value = yield if block_given?
      @backend.add key, value, timeout
    end
  end

  # Tries to get a value from the cache, and raise an ArgumentError if it fails.
  # * +key+: the key to get
  def get_or_raise key
    @backend.get key do
      raise ArgumentError, "#{key} is not cached"
    end
  end

  # Adds an item to the cache
  # * +key+: an unique identifier (in the cache)
  # * +value+: value
  # * +timeout+: time in seconds after the value will be purged from cache
  def add key, value, timeout = @timeout
    @backend.add key, value, timeout
  end

  # Removes a given key or keys from the cache. Pass is a key or a block
  # * +key+: remove this key from the cache
  # * block: it'll yield the passed block for each item
  def remove key = nil
    if block_given?
      @backend.remove { |k,v| yield k,v }
    else
      @backend.remove key
    end
  end
  alias :delete :remove
  alias :invalidate :delete

  # Converts the cache to a hash containing all stored values as key-value
  # pairs.
  def to_hash
    h = {}
    @backend.remove do |k, v|
      h[k] = v unless h.has_key? k
      false
    end
    h
  end

  # Clears the cache (removes all stored items)
  def clear
    @backend.clear
  end
  alias :erase :clear
end
