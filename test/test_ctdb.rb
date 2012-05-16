# require 'test/unit'
# require 'turn'
# require 'yaml'
# 
# dir = File.dirname(__FILE__)
# $:.unshift(File.join(dir, '..', 'lib'))
# require 'ctdb'
# 
# require File.join(dir, 'test_helper')
# require File.join(dir, "test_ct_date.rb")
# require File.join(dir, "test_ct_session.rb")
# require File.join(dir, "test_ct_table.rb")
# require File.join(dir, "test_ct_field.rb")

# class TestSuiteCTDB < Test::Unit::TestSuite
#   include TestHelper
# 
#   def self.suite
#     s = new(self.class.name)
#     s << TestCTDate.suite
#     s << TestCTSession.suite
#     s << TestCTTable.suite
#     s << TestCTField.suite
#     return s
#   end
# 
#   def run(*args)
#     super
#     assert_nothing_raised do
#       pattern = File.join(_c[:table_path], "#{_c[:table_name]}.{idx,dat}")
#       Dir[pattern].each { |file| File.delete(file) }
#     end
#   end
# 
# end

# class TestCT < Test::Unit::TestCase

  # def setup
  #   @table_path = File.expand_path(File.dirname(__FILE__))
  #   @table_name = "test_ctdb_sdk"
  #   @index_name = "index_t_uinteger"
  #   @c = { host: "FAIRCOMS", username: "", password: "" }
  # end
  # 
  # def teardown
  #   @table, @session, @record = nil
  # end
  # 
  # def test_xxx_delete_table
  #   assert_nothing_raised do
  #     pattern = File.join(@table_path, "#{@table_name}.{idx,dat}")
  #     Dir[pattern].each { |file| File.delete(file) }
  #   end
  # end
  # 
  # def test_create_session
  #   assert_nothing_raised { @session = CT::Session.new(CT::SESSION_CTREE) }
  #   assert_equal(false, @session.active?)
  #   assert_raise(ArgumentError){ @session.logon }
  #   assert_nothing_raised { @session.logon(@c[:host], @c[:username], @c[:password]) }
  #   assert(@session.active?)
  #   assert_nothing_raised { @session.logout }
  #   assert_equal(false, @session.active?)
  # end
  # 
  # def test_create_table
  #   assert_nothing_raised do
  #     @session = CT::Session.new(CT::SESSION_CTREE)
  #     @session.logon(@c[:host], @c[:username], @c[:password])
  #   end
  #   assert_nothing_raised { @t = CT::Table.new(@session) }
  #   assert_nothing_raised { @f = @t.add_field("t_uinteger", CT::UINTEGER, 4) }
  #   assert_instance_of(CT::Field, @f)
  #   assert_equal("t_uinteger", @f.name)
  #   assert_equal(4, @f.length)
  #   # assert_equal(CT::UINTEGER, @f.type)
  #   # assert_nothing_raised { @f.allow_nil = true }
  #   # assert_nothing_raised { @f.default = nil }
  #   assert_nothing_raised { @ndx = @t.add_index(@index_name, CT::INDEX_FIXED) }
  #   assert_nothing_raised { @ndx.add_segment(@f, nil) }#CT::SEG_SCHSEG) }
  #   
  #   assert_nothing_raised { @t.add_field("t_integer",   CT::INTEGER,     4) }
  #   assert_nothing_raised { @t.add_field("t_bool",      CT::BOOL,        1) }
  #   assert_nothing_raised { @t.add_field("t_tinyint",   CT::TINYINT,     1) }
  #   assert_nothing_raised { @t.add_field("t_utinyint",  CT::UTINYINT,    1) }
  #   assert_nothing_raised { @t.add_field("t_smallint",  CT::SMALLINT,    2) }
  #   assert_nothing_raised { @t.add_field("t_usmallint", CT::USMALLINT,   2) }
  #   assert_nothing_raised { @t.add_field("t_money",     CT::MONEY,       4) }
  #   assert_nothing_raised { @t.add_field("t_date",      CT::DATE,        4) }
  #   assert_nothing_raised { @t.add_field("t_time",      CT::TIME,        4) }
  #   assert_nothing_raised { @t.add_field("t_float",     CT::FLOAT,       4) }
  #   assert_nothing_raised { @t.add_field("t_double",    CT::DOUBLE,      8) }
  #   assert_nothing_raised { @t.add_field("t_timestamp", CT::TIMESTAMP,   8) }
  #   assert_nothing_raised { @t.add_field("t_efloat",    CT::EFLOAT,     16) }
  #   assert_nothing_raised { @t.add_field("t_binary",    CT::BINARY,    256) }
  #   assert_nothing_raised { @t.add_field("t_chars",     CT::CHARS,      32) }
  #   assert_nothing_raised { @t.add_field("t_fpstring",  CT::FPSTRING,    1) }
  #   assert_nothing_raised { @t.add_field("t_f2string",  CT::F2STRING,    2) }
  #   assert_nothing_raised { @t.add_field("t_f4string",  CT::F4STRING,    4) }
  #   assert_nothing_raised { @t.add_field("t_bigint",    CT::BIGINT,      8) }
  #   assert_nothing_raised { @t.add_field("t_number",    CT::NUMBER,      8) }
  #   assert_nothing_raised { @t.add_field("t_currency",  CT::CURRENCY,    8) }
  #   assert_nothing_raised { @t.add_field("t_pstring",   CT::PSTRING,     1) }
  #   assert_nothing_raised { @t.add_field("t_varbinary", CT::VARBINARY,   2) }
  #   assert_nothing_raised { @t.add_field("t_lvb",       CT::LVB,         4) }
  #   assert_nothing_raised { @t.add_field("t_varchar",   CT::VARCHAR,    16) }
  #   assert_nothing_raised { @t.path = @table_path }
  #   assert_nothing_raised { @t.create(@table_name, CT::CREATE_NORMAL) }
  #   assert_nothing_raised { @session.logout }
  # end
  # 
  # # def test_session_locking
  # #   assert_nothing_raised do
  # #     @session = CT::Session.new
  # #     @session.logon(@c[:host], @c[:username], @c[:password])
  # #   end
  # #   assert(@session.lock(CT::LOCK_WRITE))
  # #   assert(@session.locked?)
  # #   assert(@session.unlock)
  # #   assert_equal(false, @session.locked?)
  # #   assert_nothing_raised{ @session.lock!(CT::LOCK_WRITE) }
  # #   # assert_raise(CT::Error){ @session.lock!(CT::LOCK_READ) }
  # #   assert_nothing_raised{ @session.unlock! }
  # # end
  # 
  # # def test_session_path_prefix
  # #   prefix = "/foo/bar"
  # # 
  # #   assert_nothing_raised do 
  # #     @session = CT::Session.new(CT::SESSION_CTREE)
  # #     @session.logon(@c[:host], @c[:username], @c[:password])
  # #   end
  # # 
  # #   assert_nil(@session.path_prefix)
  # #   assert_nothing_raised { @session.path_prefix = prefix }
  # #   assert_equal(prefix, @session.path_prefix)
  # #   assert_nothing_raised { @table = CT::Table.new(@session) }
  # #   assert_nothing_raised { @table.open(@table_name, CT::OPEN_NORMAL) }
  # #   assert_nothing_raised { @table.close }
  # #   assert(@session.path_prefix = "")
  # #   assert_nil(@session.path_prefix)
  # #   assert_nothing_raised { @session.logout }
  # # end
  # 
  # def test_table
  #   assert_nothing_raised do
  #     @session = CT::Session.new(CT::SESSION_CTREE)
  #     @session.logon(@c[:host], @c[:username], @c[:password])
  #   end
  #   assert_nothing_raised { @table = CT::Table.new(@session) }
  #   assert_nil(@table.path)
  #   assert_nothing_raised { @table.path = @table_path }
  #   assert_equal(@table_path, @table.path)
  #   assert_nothing_raised { @table.open(@table_name, CT::OPEN_NORMAL) }
  #   assert_equal(@table_name, @table.name)
  #   assert_instance_of(Fixnum, @table.field_count)
  #   assert_instance_of(Array, @table.field_names)
  #   assert_equal(@table.field_count, @table.field_names.size)
  #   @table.get_fields do |field|
  #     assert_instance_of(CT::Field, field)
  #   end
  #   assert_nothing_raised { @table.close }
  #   assert_nothing_raised { @session.logout }
  # end
  # 
  # def test_table_add_records
  #   assert_nothing_raised do 
  #     @session = CT::Session.new(CT::SESSION_CTREE)
  #     @session.logon(@c[:host], @c[:username], @c[:password])
  #     @table = CT::Table.new(@session)
  #     @table.path = @table_path
  #     @table.open(@table_name, CT::OPEN_NORMAL)
  #   end
  # 
  #   load_fixture.each do |f|
  #     assert_nothing_raised { @record = CT::Record.new(@table) }
  #     assert_nothing_raised { @record.clear }
  # 
  #     assert_nothing_raised { @record.set_field_as_bool(0, f["t_bool"]) }
  #     assert_equal(f["t_bool"], @record.get_field_as_bool(0))
  # 
  #     assert_nothing_raised { @record.set_field_as_signed(1, f["t_tinyint"]) }
  #     assert_equal(f["t_tinyint"], @record.get_field_as_signed(1))
  # 
  #     assert_nothing_raised { @record.set_field_as_unsigned(2, f["t_utinyint"]) }
  #     assert_equal(f["t_utinyint"], @record.get_field_as_unsigned(2))
  # 
  #     assert_nothing_raised { @record.set_field_as_signed(3, f["t_smallint"]) }
  #     assert_equal(f["t_smallint"], @record.get_field_as_signed(3))
  # 
  #     assert_nothing_raised { @record.set_field_as_unsigned(4, f["t_usmallint"]) }
  #     assert_equal(f["t_usmallint"], @record.get_field_as_unsigned(4))
  # 
  #     assert_nothing_raised { @record.set_field_as_signed(5, f["t_integer"]) }
  #     assert_equal(f["t_integer"], @record.get_field_as_signed(5))
  # 
  #     assert_nothing_raised { @record.set_field_as_unsigned(6, f["t_uinteger"]) }
  #     assert_equal(f["t_uinteger"], @record.get_field_as_unsigned(6))
  # 
  #     # assert_nothing_raised { @record.set_field_as_date(7, f["t_date"]) }
  #     # assert_nothing_raised { @record.set_field_as_time(8, f["t_time"]) }
  #     # assert_nothing_raised { @record.set_field_as_float(9, f["t_float"]) }
  #     # assert_nothing_raised { @record.set_field_as_(10, f["t_double"]) }
  #     # assert_nothing_raised { @record.set_field_as_datetime(11, f["t_timestamp"]) }
  #     # assert_nothing_raised { @record.set_field_as_float(12, f["t_efloat"]) }
  #     # assert_nothing_raised { @record.set_field_as_binary(13, f["t_binary"]) }
  #     # assert_nothing_raised { @record.set_field_as_string(14, f["t_chars"]) }
  #     assert_nothing_raised { @record.write! }
  #   end
  #   assert_nothing_raised do
  #     @table.close
  #     @session.logout
  #   end
  # end
  # 

  # 
  # private
  # 
  #   def load_fixture
  #     @f ||= YAML.load_file(File.join(File.dirname(__FILE__), "fixture.yml"))
  #   end

# end