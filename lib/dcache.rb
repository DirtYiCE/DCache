# Main DCache class
class DCache
  # DCache's version
  VERSION = '0.0.0'

  protected
  # Declares a reader to the underlying backend.
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

  # @return [Symbol] Which DCache backend is used (can be +:list+ or +:ary+
  #   currently)
  attr_reader :backend_type
  # Current backend in use.
  attr_reader :backend

  # @return [Number] Number of cache hits
  embedded_reader :hits
  # @return [Number] Number of cache misses
  embedded_reader :misses
  # @return [Number] Number of elements stored (only if
  #   <tt>backend == :list</tt>)
  embedded_reader :stored
  alias :added :stored
  # @return [Number] Number of elements removed from cache (only if
  #   <tt>backend == :list</tt>)
  embedded_reader :removed
  alias :deleted :removed
  # @return [Number] Number of elements currently stored in cache
  embedded_reader :length
  alias :size :length
  alias :items :length
  # @return [Number, nil] Maximum number of elements that can be stored, or
  #   +nil+ if there's no limit.
  embedded_reader :max_items
  alias :max_length :max_items
  alias :max_size :max_items
  # Set maximum number of elements.  Only if <tt>backend == :list</tt>.
  # @param [Number, nil] val new maximum number of elements. If it's smaller
  #   than the amount of currently stored elements, old elements will be
  #   eliminated. If it's +nil+, there's no maximum.
  # @return [Number, nil] parameter *val*
  def max_items= val
    @backend.max_items = val
  end
  alias :max_length= :max_items=
  alias :max_size= :max_items=

  # @return [Number, nil] Default timeout for new values, or +nil+ if there's
  #   no timeout
  attr_accessor :timeout

  # Initializes DCache.
  # @param [Symbol] backend which backend to use. Currently can be +:ary+ or
  #   +:list+. +:ary+ is based on a fixed size array, and implements something
  #   like "Least Recently Added". +:list+ uses a linked list to store the
  #   elements, so it's maximum size  can be changed during runtime, and it
  #   implements a simple LRU cache.
  # @param [ ] args optional parameters passed to backend initialization. See
  #   {DCacheAry} and {DCacheList}.
  def initialize backend = :list, *args
    require "dcache_#{backend}"
    @backend_type = backend
    @backend = Kernel.const_get("DCache#{backend.capitalize}").new(*args)
    @timeout = nil
  end

  # Tries to get a value from the cache.
  # @overload get(key, default = nil)
  #   Tries to get a value from cache. If it fails, returns a default value.
  #   @param [#eql?] key the key of the value to get
  #   @param default default return value.
  #   @return the value from cache if found, *default* otherwise
  # @overload get(key) { ... }
  #   Tries to get a value from cache. If it fails, executes the block.
  #   @param [#eql?] key the key of the value to get
  #   @yield if key is not found.
  #   @return the value from cache if found, or yielded block's return value.
  def get key, default = nil
    if block_given?
      @backend.get(key) { yield }
    else
      @backend.get key, default
    end
  end

  # Tries to get a value. If it fails, add it, then return.
  # @overload get_or_add(key, value, timeout = @timeout)
  #   @param [#eql?] key the key of the value to get
  #   @param value value to set if get fails. If a block is passed, it'll be
  #     evaluated and used instead of this
  #   @param [Number, nil] timeout of the newly added item, or +nil+ if there's
  #     no timeout.
  # @overload get_or_add(key, timeout = @timeout) { ... }
  #   @param [#eql?] key the key of the value to get
  #   @param [Number, nil] timeout of the newly added item, or +nil+ if there's
  #     no timeout.
  #   @yield if *key* is not found
  #   @yieldreturn value for the key, that will be added to cache.
  # @return value from cache that it either already had, or has been added.
  def get_or_add key, *plus
    @backend.get key do
      if block_given?
        value = yield
        timeout = plus[0] or @timeout
      else
        value = plus[0]
        timeout = plus[1] or @timeout
      end
      @backend.add key, value, timeout
    end
  end

  # Tries to get a value from the cache, or raise an exception if it fails.
  # @param [#eql?] key the key to get
  # @return the value from cache.
  # @raise ArgumentError if *key* is not found in cache.
  def get_or_raise key
    @backend.get key do
      raise ArgumentError, "#{key} is not cached"
    end
  end

  # Adds an item to the cache
  # @param [#eql?] key an unique identifier (in the cache)
  # @param value the value
  # @param [Number, nil] timeout time in seconds after the value will be purged
  #   from cache. +nil+ if there's no such limit.
  # @return *value*
  def add key, value, timeout = @timeout
    @backend.add key, value, timeout
  end

  # Removes a given key or keys from the cache.
  # @overload remove(key)
  #   @param key remove this key from the cache
  # @overload remove() { |key, value| ... }
  #   @yield [key, value] for each item in the cache,
  #   @yieldparam key key of the current item.
  #   @yieldparam value value associated with the *key*.
  #   @yieldreturn [Boolean] +true+ if you want to delete this item
  # @return [nil]
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
  # @return [Hash] all cached entries as a hash.
  def to_hash
    h = {}
    @backend.remove do |k, v|
      h[k] = v unless h.has_key? k
      false
    end
    h
  end

  # Clears the cache (removes all stored items)
  # @return [nil]
  def clear
    @backend.clear
  end
  alias :erase :clear

  # Overridden inspect to list info from underlying backend aswell.
  # @return [String]
  def inspect
    s = "#<#{self.class.to_s} "
    s << get_variables_ary.map do |i|
      ss ="#{i}="
      if (i[0] == '@') then
        ss << instance_variable_get(i).inspect
      else
        ss << send(i).inspect
      end
    end.join(', ')
    s << '>'
  end

  def pretty_print p
    p.object_group(self) do
      ary = get_variables_ary
      p.seplist(ary, lambda { p.text ','}) do |i|
        p.breakable
        p.text "#{i}"
        p.text '='
        p.group(1) {
          p.breakable ''
          if i[0] == '@' then
            p.pp(self.instance_variable_get(i))
          else
            p.pp(self.send(i))
          end
        }
      end
      p.text ','
      p.breakable
      p.text "items="
      p.group(1, '{', '}') {
        prev = false
        @backend.remove do |k,v|
          p.text ',' if prev
          prev = true

          p.breakable
          p.group(1) {
            p.pp(k)
          }
          # non-standard, but i like better that way
          p.text ' => '
          p.group(1) {
            p.pp(v)
          }
          false
        end
      }
    end
  end

  private
  # @return [Array<Symbol, String>] list of instance and embedded variables.
  def get_variables_ary
    ary = ['@backend_type', '@backend', '@timeout', :length, :max_length,
           :hits, :misses]
    ary += [:stored, :removed] if @backend_type == :list
    ary
  end
end
