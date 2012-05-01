require File.dirname(__FILE__) + '/lib/ctdb'
require 'rake/extensiontask'

Rake::ExtensionTask.new('ctdb_sdk')

task :build do
  `gem build ctree.gemspec`
end

task :tag do
  `git tag v#{CTDB::Version}`
end

task :publish => [:tag, :build] do
end