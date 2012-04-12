require File.dirname(__FILE__) + '/lib/ctree'

task :build do
  `gem build ctree.gemspec`
end

task :tag do
  `git tag v#{Ctree::Version}`
end

task :publish => [:tag, :build] do
end