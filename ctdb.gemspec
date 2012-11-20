require File.dirname(__FILE__) + '/lib/ctdb/version'

Gem::Specification.new do |s|
  s.name     = "ctdb"
  s.version  = "#{CT::Version}"
  s.platform = Gem::Platform::RUBY
  s.summary  = "FairCom c-treeDB ruby SDK"
  s.description = "No fluff c extension implementing the ctdb sdk."
  s.homepage = "https://github.adfitech.com/dan/rb-ctdb"
  s.email    = "it@adfitech.com"

  s.authors = [ "Daniel Johnston" ]

  s.files       = Dir['lib/*.rb'] + 
                  Dir['ext/ctdb_sdk/*.{c,rb}']

  s.extensions  = [ 'ext/ctdb_sdk/extconf.rb' ]

end
