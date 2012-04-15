begin
  require 'ctree_sdk'
rescue LoadError
  warn "WARNING: ctree_api not loaded"
end

class Ctree
  Version = VERSION = '1.0.0'
end