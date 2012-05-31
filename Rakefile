require File.dirname(__FILE__) + '/lib/ctdb'
require 'rake/extensiontask'
require 'yard'

Rake::ExtensionTask.new('ctdb_sdk')

desc "Start the guard process."
task :guard do
  `bundle exec guard start`
end

desc "Build the RubyGem."
task :build do
  `gem build ctdb.gemspec`
end

desc "Tag the project with the current version."
task :tag do
  `git tag v#{CT::Version}`
end

task :publish => [:tag, :build] do
  sh("git push origin master")
  sh("git push git@github.adfitech.com:dan/rb-ctdb.git v#{CT::Version}")
  sh("gem inabox ctdb-#{CT::Version}.gem")
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