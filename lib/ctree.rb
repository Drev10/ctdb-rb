begin
  require 'ctree_sdk'
rescue LoadError
  warn "WARNING: ctree_sdk not loaded."
end

module Ctree
  Version = VERSION = '1.0.0'
end