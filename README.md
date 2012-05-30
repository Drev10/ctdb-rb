# Ruby c-treeDB SDK

A no fluff extension implementing the FairCom c-tree ctdb sdk.

## Installation

**OSX**

```
$ gem install ctree -- --with-faircom-dir=/path/to/osx.v10.6.64bit
```

## Contributing

* Fork the project
* Create a topic branch
* Hack away
* Submit a pull request

**Note:** Before you get started, scope out the documentation to get familiar with
the project.  You can do this by running `rake yard`, then open doc/index.html 
with your preferred browser.  Also, if your lazy like me run `rake guard`.  This 
will automagically compile the c extension after you modify the ctdb_skd.c file. 
This speeds up the process when debugging.    