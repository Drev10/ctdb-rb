# $:.unshift(File.dirname(__FILE__) + '/../lib')
# require 'ctree'
require 'test/unit'
require 'turn/autorun'

class TestCtree < Test::Unit::TestCase
  
  def setup
  end
  
  def test_init
    assert_nothing_raised{ @ct = Ctree.init }
    # assert_nothing_raised{ @ct.logout }
  end
  
end