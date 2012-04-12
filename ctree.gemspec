require File.dirname(__FILE__) + '/lib/ctree'

Gem::Specification.new do |s|
  s.name = "ctree"
  s.version = "#{Ctree::Version}"
  s.summary = "FairCom c-tree ruby api"
  
  s.authors = [ "Daniel Johnston" ]
  
  s.files       = Dir['lib/*.rb'] + Dir['ext/*.{c,h,rb}']
  s.extensions  = [ 'ext/extconf.rb' ]

end