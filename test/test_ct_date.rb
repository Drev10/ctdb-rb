require File.join(File.dirname(__FILE__), 'test_helper')

class TestCTDate < Test::Unit::TestCase

  def test_date
    assert_nothing_raised { @date = CT::Date.new(1984, 10, 1) }
    assert_equal(1984, @date.year)
    assert_equal(10, @date.month)
    assert_equal(1, @date.day)
    assert_equal(1, @date.day_of_week)
    assert(@date.leap_year?)
    # assert_equal("01/10/1984", @date.strftime(CT::DATE_MDCY))
    # assert_equal("01/10/84", @date.strptime(CT::DATE_MDY))
    # assert_equal("10/01/84", @date.strptime(CT::DATE_DMY))
    # assert_equal("19840110", @date.strptime(CT::DATE_CYMD))
    # assert_equal("841001", @date.strptime(CT::DATE_YMD))
  end
  
  # def test_date_mdcy_regex
  #   date = "01/10/1984"
  # 
  #   assert_match(CT::DATE_MDCY_REGEX, date, "mm/dd/ccyy")
  #   assert_no_match(CT::DATE_MDY_REGEX,  date, "mm/dd/yy")
  #   assert_no_match(CT::DATE_DMY_REGEX,  date, "dd/mm/yy")
  #   assert_no_match(CT::DATE_CYMD_REGEX, date, "ccyymmdd")
  #   assert_no_match(CT::DATE_YMD_REGEX,  date, "yymmdd")
  # end
  
  # def test_date_mdy_regex
  #   date = "01/10/84"
  # 
  #   assert_no_match(CT::DATE_MDCY_REGEX, date, "mm/dd/ccyy")
  #   assert_match(CT::DATE_MDY_REGEX,  date, "mm/dd/yy")
  #   assert_no_match(CT::DATE_DMCY_REGEX, date, "dd/mm/ccyy")
  #   # assert_no_match(CT::DATE_DMY_REGEX,  date, "dd/mm/yy")
  #   assert_no_match(CT::DATE_CYMD_REGEX, date, "ccyymmdd")
  #   assert_no_match(CT::DATE_YMD_REGEX,  date, "yymmdd")
  # end
  
  # def test_date_dmcy_regex
  #   date = "10/01/1984"
  # 
  #   assert_match(CT::DATE_MDCY_REGEX, date, "mm/dd/ccyy")
  #   assert_no_match(CT::DATE_MDY_REGEX,  date, "mm/dd/yy")
  #   assert_no_match(CT::DATE_DMCY_REGEX, date, "dd/mm/ccyy")
  #   assert_no_match(CT::DATE_DMY_REGEX,  date, "dd/mm/yy")
  #   assert_no_match(CT::DATE_CYMD_REGEX, date, "ccyymmdd")
  #   assert_no_match(CT::DATE_YMD_REGEX,  date, "yymmdd")
  # end
  
  # def test_date_dmy_regex
  #   date = "10/01/1984"
  # 
  #   assert_no_match(CT::DATE_MDCY_REGEX, date, "mm/dd/ccyy")
  #   assert_no_match(CT::DATE_MDY_REGEX,  date, "mm/dd/yy")
  #   assert_no_match(CT::DATE_DMCY_REGEX, date, "dd/mm/ccyy")
  #   assert_no_match(CT::DATE_DMY_REGEX,  date, "dd/mm/yy")
  #   assert_no_match(CT::DATE_CYMD_REGEX, date, "ccyymmdd")
  #   assert_no_match(CT::DATE_YMD_REGEX,  date, "yymmdd")
  # end
  
  # def test_date_cymd_regex
  #   date = "19841001"
  # 
  #   assert_no_match(CT::DATE_MDCY_REGEX, date, "mm/dd/ccyy")
  #   assert_no_match(CT::DATE_MDY_REGEX,  date, "mm/dd/yy")
  #   assert_no_match(CT::DATE_DMCY_REGEX, date, "dd/mm/ccyy")
  #   assert_no_match(CT::DATE_DMY_REGEX,  date, "dd/mm/yy")
  #   assert_no_match(CT::DATE_CYMD_REGEX, date, "ccyymmdd")
  #   assert_no_match(CT::DATE_YMD_REGEX,  date, "yymmdd")
  # end
  
  # def test_date_ymd_regex
  #   date = "840110"
  # 
  #   assert_no_match(CT::DATE_MDCY_REGEX, date, "mm/dd/ccyy")
  #   assert_no_match(CT::DATE_MDY_REGEX,  date, "mm/dd/yy")
  #   assert_no_match(CT::DATE_DMCY_REGEX, date, "dd/mm/ccyy")
  #   assert_no_match(CT::DATE_DMY_REGEX,  date, "dd/mm/yy")
  #   assert_no_match(CT::DATE_CYMD_REGEX, date, "ccyymmdd")
  #   assert_no_match(CT::DATE_YMD_REGEX,  date, "yymmdd")
  # end

end