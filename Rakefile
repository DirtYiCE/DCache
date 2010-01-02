# -*- mode: ruby; encoding: utf-8 -*-

require 'rubygems'
require 'rake'

DLEXT = Config::CONFIG['DLEXT']
EXTS = %w(ary list)
EXT_FILES = EXTS.map do |i|
  "lib/dcache_#{i}.#{DLEXT}"
end

begin
  require 'jeweler'
  Jeweler::Tasks.new do |gem|
    gem.name = "dcache"
    gem.summary = "A simple caching gem"
    gem.description = %Q{
A simple caching gem using multiple backends written in C.
}
    gem.email = "DirtY.iCE.hu@gmail.com"
    gem.homepage = "http://github.com/DirtYiCE/DCache"
    gem.authors = ["Kővágó, Zoltán"]
    gem.add_development_dependency "rspec", ">= 1.2.9"
    gem.add_development_dependency "yard", ">= 0.5"
    gem.has_rdoc = false
    gem.extensions = EXTS.map do |i|
      "ext/dcache_#{i}/extconf.rb"
    end
    gem.post_install_message = %Q{
** Please build the documentation using yardoc.
}

    # gem is a Gem::Specification... see http://www.rubygems.org/read/chapter/20 for additional settings
  end
  Jeweler::GemcutterTasks.new
rescue LoadError
  puts "Jeweler (or a dependency) not available. Install it with: gem install jeweler"
end

require 'spec/rake/spectask'
Spec::Rake::SpecTask.new(:spec) do |spec|
  spec.libs << 'lib' << 'spec'
  spec.spec_files = FileList['spec/**/*_spec.rb']
end

Spec::Rake::SpecTask.new(:rcov) do |spec|
  spec.libs << 'lib' << 'spec'
  spec.pattern = 'spec/**/*_spec.rb'
  spec.rcov = true
end

task :spec => :check_dependencies
task :spec => EXT_FILES

task :default => :spec

begin
  require 'yard'
  YARD::Rake::YardocTask.new
rescue LoadError
  task :yardoc do
    abort "YARD is not available. In order to run yardoc, you must: sudo gem install yard"
  end
end

# build extensions
desc "Build native extensions"
task :make => EXT_FILES

EXTS.each do |e|
  file "lib/dcache_#{e}.#{DLEXT}" => "ext/dcache_#{e}/dcache_#{e}.c" do
    Dir.chdir "ext/dcache_#{e}" do
      ruby 'extconf.rb'
      sh 'make'
    end
    FileUtils.copy "ext/dcache_#{e}/dcache_#{e}.#{DLEXT}", "lib/"
  end
end
