require 'test/unit'
require 'turn'

$:.unshift(File.dirname(__FILE__) + '/../lib')
require 'ctree'

class TestCtree < Test::Unit::TestCase

  def setup
    @table_path = "/tmp"
    @table_name = "test_ctree_sdk"
    @index      = "index_fstring"
    @c = { host: "FAIRCOMS", username: "", password: "" }
  end

  def test_session
    assert_nothing_raised { @session = Ctree::Session.new }
    assert_equal(false, @session.active?)
    assert_raise(ArgumentError){ @session.logon }
    assert_nothing_raised { @session.logon(@c[:host], @c[:username], @c[:password]) }
    assert(@session.active?)
    assert_nothing_raised { @session.logout }
    assert_equal(false, @session.active?)
  end

  def test_session_locking
    assert_nothing_raised do
      @session = Ctree::Session.new
      @session.logon(@c[:host], @c[:username], @c[:password])
    end
    assert(@session.lock(Ctree::LOCK_WRITE))
    assert(@session.locked?)
    assert(@session.unlock)
    assert_equal(false, @session.locked?)
    assert_nothing_raised{ @session.lock!(Ctree::LOCK_WRITE) }
    # assert_raise(Ctree::Error){ @session.lock!(Ctree::LOCK_READ) }
    assert_nothing_raised{ @session.unlock! }
  end

  def test_session_path_prefix
    assert_nothing_raised { @session = Ctree::Session.new }
    assert_nil(@session.path_prefix)
    assert_nothing_raised { @session.path_prefix = "/foo/bar" }
    assert_equal("/foo/bar", @session.path_prefix)
    assert(@session.path_prefix = "")
    assert_nil(@session.path_prefix)
    # assert(@session.path_prefix = nil)
    # assert_nil(@session.path_prefix)
  end

  def test_table
    assert_nothing_raised do
      @session = Ctree::Session.new
      @session.logon(@c[:host], @c[:username], @c[:password])
    end
    assert_nothing_raised { @table = Ctree::Table.new(@session) }
    assert_nil(@table.path)
    assert_nothing_raised { @table.path = @table_path }
    assert_equal(@table_path, @table.path)
    assert_nothing_raised { @table.open(@table_name) }
    assert_equal(@table_name, @table.name)
    assert_instance_of(Array, @table.field_names)
    assert_equal(5, @table.field_names.size)
    assert_nothing_raised { @table.close }
    assert_nothing_raised { @session.logout }
  end

  def test_record
    assert_nothing_raised do 
      @session = Ctree::Session.new
      @session.logon(@c[:host], @c[:username], @c[:password])
      @table = Ctree::Table.new(@session)
      @table.path = @table_path
      @table.open(@table_name)
    end
    assert_nothing_raised { @record = Ctree::Record.new(@table) }

    assert_nothing_raised{ @record.first }
    # assert_equal("foo", @record.field('t_fstring'))
    # assert_equal(0, @record.field('t_int2'))

    assert_nothing_raised{ @record.next }
    # assert_equal("bar", @record.field('t_fstring'))
    # assert_equal(1, @record.field('t_int2'))

    assert_nothing_raised{ @record.next }
    # assert_equal(2, @record.field('t_int2'))
    # assert_equal("faz", @record.field('t_fstring'))

    assert_nothing_raised{ @record.next }
    # assert_equal(3, @record.field('t_int2'))
    # assert_equal("baz", @record.field('t_fstring'))

    assert_nil(@record.next)
  end

  def test_record_getters_and_setters
    assert_nothing_raised do 
      @session = Ctree::Session.new
      @session.logon(@c[:host], @c[:username], @c[:password])
      @table = Ctree::Table.new(@session)
      @table.path = @table_path
      @table.open(@table_name)
    end
    assert_nothing_raised do
      @record = Ctree::Record.new(@table)
      @record.first
    end
    assert_equal("foo", @record.get_field('t_fstring'))
    assert_nothing_raised { @record.set_field('t_fstring', 'test') }
    assert_equal("test", @record.get_field('t_fstring'))
    assert_equal(2652290, @record.get_field('t_int4'))
    assert_nothing_raised { @record.set_field('t_int4', 2652299) }
    assert_equal(2652299, @record.get_field('t_int4'))
  end

  def test_record_indexes
    assert_nothing_raised do 
      @session = Ctree::Session.new
      @session.logon(@c[:host], @c[:username], @c[:password])
      @table = Ctree::Table.new(@session)
      @table.path = @table_path
      @table.open(@table_name)
    end
    assert_nothing_raised { @record = Ctree::Record.new(@table) }
    # assert_nothing_raised{ @record.default_index = @index }
    # assert_equal(@index, @record.default_index)
    # assert_nothing_raised{ @record.default_index = :"#{@index}" }
    # assert_equal(@index, @record.default_index)
  end

  # def test_record_finders
  # end

end