begin
  require 'ctdb_sdk'
rescue LoadError
  warn "WARNING: ctdb_sdk not loaded."
end

require 'ctdb/version'
require 'ctdb/field'
require 'ctdb/record'
