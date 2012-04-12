require File.dirname(__FILE__) + '/lib/ctree'

Gem::Specification.new do |s|
  s.name     = "ctree"
  s.version  = "#{Ctree::Version}"
  s.platform = Gem::Platform::RUBY
  s.summary  = "FairCom c-tree ruby api"
  
  s.authors = [ "Daniel Johnston" ]
  
  s.files       = Dir['lib/*.rb'] + Dir['ext/*.{c,h,rb}']
  s.extensions  = [ 'ext/ctree_api/extconf.rb' ]

  s.add_development_dependency "rake-compiler"
  s.add_development_dependency "turn"
end