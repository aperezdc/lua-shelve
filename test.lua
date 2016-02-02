
local function printf(fmt, ...)
	io.stdout:write(string.format(fmt, unpack(arg)))
end

package.cpath = "./?.so"
local shelve = assert(require("shelve"))
print("Opening 'db'")

local db = assert(shelve.open("test.db"))

a = "one"
b = "two"
c = "three"

db.num = 1234567890 -- a number
db.str = "a string" -- a string
db.t1  = {}         -- an empty table
db.t2  = { s="S" }  -- table with one element
db.t3  = { a,b,c }  -- indexed table, multiple elements

db.nested =         -- nested tables
   {
      level = a,
      nest  = {
         level = b,
         nest  = {
            level = c,
            nest  = "nothing"
         }
      }
   }

printf("Number encoding... ")
assert(type(db.num) == "number")
assert(db.num == 1234567890)
printf("ok\nString encoding... ")
assert(db.str == "a string")
printf("ok\nTable encoding... ")
assert(type(db.t1) == "table")
assert(type(db.t2) == "table")
assert(type(db.t3) == "table")
printf("ok\nData integrity... ")
assert(db.t2.s == "S")
assert(db.t3[1] == a)
assert(db.t3[2] == b)
assert(db.t3[3] == c)
t = db.nested
assert(type(t) == "table")
assert(t.level == a)
assert(type(t.nest) == "table")
assert(t.nest.level == b)
assert(type(t.nest.nest) == "table")
assert(t.nest.nest.level == c)
assert(type(t.nest.nest.nest) == "string")
assert(t.nest.nest.nest == "nothing")
printf("ok\nLarge file storing... ")

fd, err = io.open("LICENSE", "r")
if (not fd) then
   print(err)
   os.exit()
end
lgpl_license = fd:read("*a")
fd:close()

db.lic = lgpl_license
assert(db.lic == lgpl_license)
printf("ok\n... all tests successfully passed ...\n")
