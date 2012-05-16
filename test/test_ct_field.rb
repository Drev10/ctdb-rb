require File.join(File.dirname(__FILE__), 'test_helper')

class TestCTField < Test::Unit::TestCase
  include TestHelper

  def setup
    session = CT::Session.new(CT::SESSION_CTREE)
    session.logon(_c[:engine], _c[:user], _c[:password])
    @table = CT::Table.new(session)
    @table.path = _c[:table_path]
    @table.open(_c[:table_name], CT::OPEN_NORMAL)
  end

  def teardown
    @table.close
  end

  def test_type
    assert_nothing_raised { @field = @table.get_field_by_name("t_uinteger") }
    assert_equal(0, @field.number)
    assert_equal(CT::UINTEGER, @field.type)
    assert_equal(4, @field.length)
    assert_instance_of(Hash, @field.properties)

    assert_nothing_raised { @field = @table.get_field_by_name("t_integer") }
    assert_equal(1, @field.number)
    assert_equal(CT::INTEGER, @field.type)
    assert_equal(4, @field.length)

    assert_nothing_raised { @field = @table.get_field_by_name("t_bool") }
    assert_equal(2, @field.number)
    assert_equal(CT::BOOL, @field.type)
    assert_equal(1, @field.length)

    assert_nothing_raised { @field = @table.get_field_by_name("t_tinyint") }
    assert_equal(3, @field.number)
    assert_equal(CT::TINYINT, @field.type)
    assert_equal(1, @field.length)

    assert_nothing_raised { @field = @table.get_field_by_name("t_utinyint") }
    assert_equal(4, @field.number)
    assert_equal(CT::UTINYINT, @field.type)
    assert_equal(1, @field.length)

    assert_nothing_raised { @field = @table.get_field_by_name("t_smallint") }
    assert_equal(5, @field.number)
    assert_equal(CT::SMALLINT, @field.type)
    assert_equal(2, @field.length)

    assert_nothing_raised { @field = @table.get_field_by_name("t_usmallint") }
    assert_equal(6, @field.number)
    assert_equal(CT::USMALLINT, @field.type)
    assert_equal(2, @field.length)

    assert_nothing_raised { @field = @table.get_field_by_name("t_money") }
    assert_equal(7, @field.number)
    assert_equal(CT::MONEY, @field.type)
    assert_equal(4, @field.length)

    assert_nothing_raised { @field = @table.get_field_by_name("t_date") }
    assert_equal(CT::DATE, @field.type)
    assert_equal(4, @field.length)

    assert_nothing_raised { @field = @table.get_field_by_name("t_time") }
    assert_equal(CT::TIME, @field.type)
    assert_equal(4, @field.length)

    assert_nothing_raised { @field = @table.get_field_by_name("t_float") }
    assert_equal(CT::FLOAT, @field.type)
    assert_equal(4, @field.length)

    assert_nothing_raised { @field = @table.get_field_by_name("t_double") }
    assert_equal(CT::DOUBLE, @field.type)
    assert_equal(8, @field.length)

    assert_nothing_raised { @field = @table.get_field_by_name("t_timestamp") }
    assert_equal(CT::TIMESTAMP, @field.type)
    assert_equal(8, @field.length)

    assert_nothing_raised { @field = @table.get_field_by_name("t_efloat") }
    assert_equal(CT::EFLOAT, @field.type)
    assert_equal(16, @field.length)

    assert_nothing_raised { @field = @table.get_field_by_name("t_binary") }
    assert_equal(CT::BINARY, @field.type)
    assert_equal(256, @field.length)

    assert_nothing_raised { @field = @table.get_field_by_name("t_chars") }
    assert_equal(CT::CHARS, @field.type)
    assert_equal(32, @field.length)

    assert_nothing_raised { @field = @table.get_field_by_name("t_fpstring") }
    assert_equal(CT::FPSTRING, @field.type)
    assert_equal(1, @field.length)

    assert_nothing_raised { @field = @table.get_field_by_name("t_f2string") }
    assert_equal(CT::F2STRING, @field.type)
    assert_equal(2, @field.length)

    assert_nothing_raised { @field = @table.get_field_by_name("t_f4string") }
    assert_equal(CT::F4STRING, @field.type)
    assert_equal(4, @field.length)

    assert_nothing_raised { @field = @table.get_field_by_name("t_bigint") }
    assert_equal(CT::BIGINT, @field.type)
    assert_equal(8, @field.length)

    assert_nothing_raised { @field = @table.get_field_by_name("t_number") }
    assert_equal(CT::NUMBER, @field.type)
    assert_equal(8, @field.length)

    assert_nothing_raised { @field = @table.get_field_by_name("t_currency") }
    assert_equal(CT::CURRENCY, @field.type)
    assert_equal(8, @field.length)

    assert_nothing_raised { @field = @table.get_field_by_name("t_pstring") }
    assert_equal(CT::PSTRING, @field.type)
    assert_equal(1, @field.length)

    assert_nothing_raised { @field = @table.get_field_by_name("t_varbinary") }
    assert_equal(CT::VARBINARY, @field.type)
    assert_equal(2, @field.length)

    assert_nothing_raised { @field = @table.get_field_by_name("t_lvb") }
    assert_equal(CT::LVB, @field.type)
    assert_equal(4, @field.length)
    
    assert_nothing_raised { @field = @table.get_field_by_name("t_varchar") }
    assert_equal(CT::VARCHAR, @field.type)
    assert_equal(16, @field.length)
  end

end