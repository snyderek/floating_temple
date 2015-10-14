local begin_tran = floating_temple.begin_tran
local end_tran = floating_temple.end_tran
local shared = floating_temple.shared

begin_tran()
local num = shared["num"]
if num == nil then
  num = 1
else
  num = num + 1
end
shared["num"] = num
end_tran()

print("My unique number is", num)
