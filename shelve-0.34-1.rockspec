local V = "0.34"
local R = 1

package = "shelve"
version = V.."-"..R

source = {
   url = "git://github.com/aperezdc/lua-shelve";
   tag = "v"..V;
}

description = {
   summary = "Serialization and on-disk persistence for Lua values";
   detailed = [[
      The shelve module implemets a persistent table-like storage
      data structure which can store any Lua data values, which are
      marshalled and unmarshalled transparently. Persistence uses
      the GDBM or NDBM libraries.
   ]];
   license = "LGPLv2";
}

dependencies = {
   "lua ~= 5.0";
}
external_dependencies = {
   LIBGDBM = {
      header = "gdbm.h";
   };
}

build = {
   type = "builtin";
   modules = {
      shelve = {
         sources = {
            "marshal.c";
            "shelve.c";
         };
         libraries = {
            "gdbm";
         };
      };
   };
}
