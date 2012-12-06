module CT
  class Field
  
    def inspect
      "<CT::Field#{object_id} @name=\"#{self.name}\" @type=\"#{self.human_type}\">"
    end

    def string?
      self.type == CT::CHARS || self.type == CT::FPSTRING || 
          self.type == CT::F2STRING || self.type == CT::F4STRING ||
          self.type == CT::PSTRING || self.type == CT::VARCHAR ||
          self.type == CT::LVB
    end

    def integer?
      self.type == CT::TINYINT || self.type == CT::UTINYINT || 
          self.type == CT::SMALLINT || self.type == CT::USMALLINT ||
          self.type == CT::INTEGER || self.type == CT::UINTEGER ||
          self.type == CT::BIGINT || self.type == NUMBER
    end

    def date?
      self.type == CT::DATE
    end

    def human_type
      case self.type
      when CT::BOOL       then "BOOL" 
      when CT::TINYINT    then "TINYINT"
      when CT::UTINYINT   then "UTINYINT"
      when CT::SMALLINT   then "SMALLINT"
      when CT::USMALLINT  then "USMALLINT"
      when CT::INTEGER    then "INTEGER"
      when CT::UINTEGER   then "UINTEGER"
      when CT::MONEY      then "MONEY"
      when CT::DATE       then "DATE"
      when CT::TIME       then "TIME"
      when CT::FLOAT      then "FLOAT"
      when CT::DOUBLE     then "DOUBLE"
      when CT::TIMESTAMP  then "TIMESTAMP"
      when CT::EFLOAT     then "EFLOAT"
      when CT::CHARS      then "CHARS"
      when CT::FPSTRING   then "FPSTRING"
      when CT::F2STRING   then "F2STRING"
      when CT::F4STRING   then "F4STRING"
      when CT::BIGINT     then "BIGINT"
      when CT::NUMBER     then "NUMBER"
      when CT::CURRENCY   then "CURRENCY"
      when CT::PSTRING    then "PSTRING"
      when CT::VARBINARY  then "VARBINARY"
      when CT::LVB        then "LVB"
      when CT::VARCHAR    then "VARCHAR"
      else
        "WTF?"
      end
    end

  end
end
