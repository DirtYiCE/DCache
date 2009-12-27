= DCache

* FIX (url)

== DESCRIPTION:

A simple caching gem.

== FEATURES:

* Two different backends
* Cached entries can have a timeout
* Cache structures are either allocated at once (+:ary+ backend) or grow/shrink
  dynamically (+:list+ backend)
* Backends are written in C for speed

== PROBLEMS:

* ?

== SYNOPSIS:

Initialize a cache using +:ary+ backend, and storing 1000 values
  require "dcache"
  c = DCache.new :ary, 1000
Put some value into it:
  c.add :key, "value"
  c.add :key2, 34
  c.add [1, 2, 3], [4, 5, 6]         # key/value can be any ruby type
  c.add :timeout, "of course", 60    # will be purged after 1 minute
Retrieve values:
  c.get :key                         # => "value"
  c.get :not_stored                  # => nil
  c.get :not_stored, "default value" # => "default_value"
  c.get(:not_stored) { puts "Ouch" } # will print "Ouch"
  c.get_or_add :foo, 3               # => 3
  c.get :foo                         # => 3
  c.get_or_raise :not_stored         # will raise ArgumentError
  c.get :timeout                     # => "of course"
  sleep 60
  c.get :timeout                     # => nil
Remove an element:
  c.delete :key
  c.get :key                         # => nil
  c.get :key2                        # => 34
Clear cache:
  c.clear
  c.get :key2                        # => nil
Get statistics:
  [c.hits, c.misses]                 # => [4, 8]
  # also [c.stored, c.removed] with :list backend


== REQUIREMENTS:

* Ruby 1.9 (may work with 1.8)
* Development dependencies:
  * Hoe-2.4
  * hoe-git-1.3.0
  * RSpec-1.2

== INSTALL:

After checking out install it by running:
  $ rake install_gem
This will install it into the system, so you'll need the root
password. Alternatively you can run
  $ rake gem
which will build a gem, and put it into pkg/.

== DEVELOPERS:

After checking out the source, run:

  $ rake newb

This task will install any missing dependencies, run the tests/specs,
and generate the RDoc.

== LICENSE:

(The MIT License)

Copyright (c) 2009 Kővágó, Zoltán

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
'Software'), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED 'AS IS', WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
