# -*- mode: ruby; coding: utf-8 -*-

require 'rubygems'
require 'hoe'

SOEXT = Config::CONFIG["DLEXT"]
EXTS = ["ary", "list"]
EXT_FILES = EXTS.map do |i|
  "lib/dcache_#{i}.#{SOEXT}"
end

Hoe.spec 'dcache' do
  developer("Kővágó, Zoltán", "DirtY.iCE.hu@gmail.com")

  spec_extras[:extensions] = EXTS.map do |i|
    "ext/dcache_#{i}/extconf.rb"
  end

  extra_dev_deps << ["rspec", ">=1.2"]

  clean_globs << EXT_FILES
  EXTS.each do |i|
    clean_globs << "ext/dcache_#{i}/Makefile"
    clean_globs << "ext/dcache_#{i}/dcache_#{i}.o"
    clean_globs << "ext/dcache_#{i}/dcache_#{i}.#{SOEXT}"
    clean_globs << "lib/dcache_#{i}.#{SOEXT}"
  end
  clean_globs.flatten!

  self.rspec_options = [ "--color" ]
end

task :spec => EXT_FILES
desc "Builds native extensions"
task :build => EXT_FILES

EXTS.each do |e|
  files = [ "ext/dcache_#{e}/extconf.rb", "ext/dcache_#{e}/dcache_#{e}.c" ]
  tfile = "lib/dcache_#{e}.#{SOEXT}"

  file tfile => files do
    Dir.chdir "ext/dcache_#{e}" do
      ruby "extconf.rb"
      sh "make"
    end
    FileUtils.copy "ext/dcache_#{e}/dcache_#{e}.#{SOEXT}", tfile
  end
end

# vim: syntax=ruby
