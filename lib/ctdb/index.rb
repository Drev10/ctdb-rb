module CT
  class Index

    # Retrieve all fields associated with the index.
    # @return [Array]
    def field_names
      self.segments.collect { |s| s.field_name }
    end

    def inspect
      "<CT::Index#{object_id} @name=\"#{self.name}\" @field_names=#{self.field_names}>"
    end

  end
end
