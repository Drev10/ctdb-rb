# rb-ctdb

A no fluff extension implementing the FairCom c-tree ctdb sdk.

## Install

    gem install ctdb

### Session

A c-treeACE session can be started by creating an instace of `CT::Session` and
calling `#logon`:

```ruby
session = CT::Session.new(CT::SESSION_CTREE)
session.logon("host", "user", "pass")
```

### Table

Once a session is active, you can begin to open tables.

```ruby
table = CT::Table.new(session)
table.path = "/path/to/table"
table.open("table_name", CT::OPEN_NORMAL)
```

### Record

Now that we have a table open, lets locate a record based on a unique index.

```ruby
record = CT::Record.new(table)
record.clear
record.set_default_index("foo_ndx")
record.set_field_as_unsigned('id', 123)
record.find(CT::FIND_EQ)

puts record.get_field_as_unsigned('id) # => 123
```


### Cleanup

Be sure to close any tables and the session.

```ruby
table.close
session.logout
```

### Contributing

* Fork the project
* Create a topic branch
* Hack away
* Submit a pull request

**Note:** Before you get started, scope out the documentation to get familiar with
the project.  You can do this by running `rake yard`, then open doc/index.html 
with your preferred browser.  Also, if your lazy like me run `rake guard`.  This 
will automagically compile the c extension after you modify the ctdb_skd.c file. 
This speeds up the process when debugging.    