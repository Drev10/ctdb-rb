require File.dirname(__FILE__) + '/lib/ctdb'
require 'rake/extensiontask'
require 'yard'

Rake::ExtensionTask.new('ctdb_sdk')

desc "Build the RubyGem."
task :build do
  `gem build ctree.gemspec`
end

desc "Tag the project with the current version."
task :tag do
  `git tag v#{CT::Version}`
end

task :publish => [:tag, :build] do
end

desc "Compile and run the test fixture program."
task :fixture do
  dir = File.join(File.dirname(__FILE__), 'test')
  # `SRCDIR=#{dir} OUTDIR=#{dir} make -f #{dir}/Makefile`
  `make -f#{dir}/Makefile`
end

YARD::Rake::YardocTask.new do |t|
  t.files = ['lib/ctdb.rb', 'ext/**/*.c']
end