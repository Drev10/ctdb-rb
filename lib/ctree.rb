begin
  require 'ctree_api'
rescue LoadError
  warn "beep beep"
end

class Ctree
  Version = VERSION = '1.0.0'
end