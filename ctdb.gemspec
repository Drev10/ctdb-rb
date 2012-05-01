require File.dirname(__FILE__) + '/lib/ctdb'

Gem::Specification.new do |s|
  s.name     = "ctdb"
  s.version  = "#{CTDB::Version}"
  s.platform = Gem::Platform::RUBY
  s.summary  = "FairCom ruby SDK"

  s.authors = [ "Daniel Johnston" ]

  s.files       = Dir['lib/*.rb'] + 
                  Dir['ext/ctree_sdk/*.{c,rb}']

  s.extensions  = [ 'ext/ctree_sdk/extconf.rb' ]

  s.add_development_dependency "rake-compiler"
  s.add_development_dependency "turn"
  s.add_development_dependency "guard"
end