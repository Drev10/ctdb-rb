require 'test/unit'
require 'turn'

$:.unshift(File.dirname(__FILE__) + '/../lib')
require 'ctree'

class TestCtree < Test::Unit::TestCase

  def setup
    @table_name = "custmast"
    @c = { host: "FAIRCOMS", username: "", password: "" }
  end

  def test_session
    assert_nothing_raised { @session = Ctree::Session.new }
    assert_equal(false, @session.active?)
    assert_nothing_raised { @session.logon(@c[:host], @c[:username], @c[:password]) }
    assert(@session.active?)
    assert_nothing_raised { @session.logout }
    assert_equal(false, @session.active?)
  end

  def test_table
    assert_nothing_raised do 
      @session = Ctree::Session.new
      @session.logon(@c[:host], @c[:username], @c[:password])
    end
    assert_nothing_raised { @table = Ctree::Table.new(@session) }
    assert_nil(@table.path)
    # assert_nil(@table.name)
    assert_nothing_raised { @table.open(@table_name) }
    assert_equal(@table_name, @table.name)
    assert_nothing_raised { @table.close }
    assert_nothing_raised { @session.logout }
  end

  def test_record
    assert_nothing_raised do 
      @session = Ctree::Session.new
      @session.logon(@c[:host], @c[:username], @c[:password])
      @table = Ctree::Table.new(@session)
    end

  end

end