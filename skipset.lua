package.cpath = "luaclib/?.so;"
package.path = "?.lua;"
local skipset = require "skipset.c"

local set = skipset()


print(set:count(), #set)
for i=100,110 do
    print(set:insert(i), i, #set)
end
for i=100,110 do
    print(set:insert(i), i, #set)
end
set:delete_byrank(1)
set:delete(110)
for i=100,110 do
    print(set:rank(i),i, #set)
end
print(set:byrank(4))
print(set:count(), #set)

for i=1,10 do
    print(i, set:byrank(math.random(1, #set)))
end