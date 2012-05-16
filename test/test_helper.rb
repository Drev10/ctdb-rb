require 'test/unit'
require 'turn'

$:.unshift(File.join(File.dirname(__FILE__), '..', 'lib'))
require 'ctdb'

module TestHelper

  def _c
    @_c ||= begin
      { engine: "FAIRCOMS", 
        user: "", 
        password: "",
        table_path: File.dirname(__FILE__),
        table_name: 'test_ctdb_sdk',
        index_name: 't_uinteger_ndx' }
    end
  end

end
