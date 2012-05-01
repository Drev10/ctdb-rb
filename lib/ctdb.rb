begin
  require 'ctdb_sdk'
rescue LoadError
  warn "WARNING: ctdb_sdk not loaded."
end

module Ctree
  Version = VERSION = '1.0.0'
end