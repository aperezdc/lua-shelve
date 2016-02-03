# Lua Shelve Module

This is a Lua module that uses either GNU Gdbm or the more standard Ndbm
database libraries to store  data in files, but this data is saved in
a special way: all standard Lua datatypes may be directly stored and
retrieved in its original form and  contents, including Lua tables. A shelf
file is a special kind of userdata that behaves similarly to a Lua table, so
its contents are accessed in a table- like fashion. Quick example:

```lua
-- Create data
my_table = {
  name = "This is a string within a table",
  level= 1,
  is_at_level_one = true,
  another_table = {
    name = "This is a nested table",
    level= 2,
    is_at_level_one = false
  }
}

-- Open a 'shelve' file
file = shelve.open("data")

-- This encodes & stores the above table
-- Dotted and bracketed syntax may be used interchangeably
file["my_table"] = my_table
file.title = "File containing a string and a table"

-- Show contents of the file in (key, value) form
for key in file() do
  print("(" .. key .. ", " .. db[key] .. ")")
end
```

The name of the module, `shelve`, is taken from the Python's module
with the same name, as both work in a similar way. The nuts & bolts
of the module are coded directly in C, so operation speed is fairly
fast.


## Requirements

In order to build and use the this Lua module, you will need the following:

- Working C compiler, GCC recommended.
- GNU DBM (aka Gdbm) or the standard Unix Ndbm. If you have the old Unix DBM
  please upgrade to Gdbm or Ndbm. Gdbm is better.
- Lua 5.0 beta or above, including headers and libraries.
- GNU Make (ok, you may also  build manually...). I don't know whether other
  versions of Make will work.

If you want to compile the module as a dynamically loadable library
(plugin) you will also need the following:

- Lua interpreter with `loadlib(`" support. Most modern operating systems
  (Linux, MacOS X, Windows, BSD...) can handle this.


## Operation

To gain access to a data file, you must open it first. The module defines
an `open()` function that receives two parameters:

```lua
file_handle = shelve.open(file_name, access_mode)
```

- `file_name` is the path of the file you want to open.
- `access_mode` is a string describing how the file should be accessed.
  `"r"` means readonly, and any other string will will be interpreted
  as read+write access. If you don't pass the second parameter,
  read+write access is assumed. Note that the mode in which a file is
  opened is important: a writer needs exclusive access to the file, but
  various readers may read it concurrently. When you need only read-access,
  please use this option,  and you will get better response when several
  processes/threads are working with the same data file.

In order to retain the structure and type information of the data, it must be
encoded with this  information included. The result of coding is a stream of
bytes. The `shelve` data files are standard Gdbm/Ndbm files that contain
these encoded bytestreams as data, and the specified identifier as key of
the data, so the following Lua statement:

```lua
file["a number"] = 12345
```

Will use the string `a number` as key and the encoded form of the `12345`
numeric value as data in the Gdbm/Ndbm file.

The opposite action:

```lua
num = file["a number"]
```

Will look-up for the `a number` key in the Gdbm/Ndbm file. If the key does
*not* exist, a `nil` value is returned, otherwise the bytestream associated
with the key will be read and decoded in order to return the original Lua
structure.

To delete a *(key, data)* pair from  the file, just assign `nil` to the key.
Example:

```
file = shelve.open("test")
file.number = 12345  -- this assignment defines "number"
file.number = nil    -- this assignment deletes "number"
```

Indexing a shelve file with an undefined key will always return `nil`.
Example:

```
file = shelve.open("test")
file.number = 12345   -- this assignment defines "number"
print(file.number)    -- prints "12345"
file.number = nil     -- this assignment deletes "number"
print(file.number)    -- prints "nil"
print(file.undefined) -- prints "nil", again
```

In order to close a shelf, just assign let it be garbage-collected When Gdbm
is used as backend and the file was open in  read+write mode, it will be
reorganized on close.


## Extras

The module also defines two standalone functions that perform the
encoding/decoding of data:

```lua
encoded = shelve.marshal(value)
```

Encodes `value` and returns the encoded bytestream as a Lua string.

```lua
value = shelve.unmarshal(encoded):
```

Takes an `encoded` Lua string and returns the decoded Lua value.

The relantionship between these two functions is that one is the
inverse of the other, so:

```lua
data1 == shelve.unmarshal(shelve.marshal(data1))  -- true
data2 == shelve.marshal(shelve.unmarshal(data2))  -- true
```

You may use these functions to manually encode and decode data.
Note that `shelve.marshal()` adds a special mark to the end of an
encoded bytestream: decoding the concatenation of two encoded
values will only return the first one.
