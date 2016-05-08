local begin_tran = floating_temple.begin_tran
local end_tran = floating_temple.end_tran
local shared = floating_temple.shared

begin_tran()
if shared["num"] == nil then
  shared["num"] = 0
end
end_tran()

local lst = {}

for i=1,10 do
  begin_tran()
  local num = shared["num"] + 1
  shared["num"] = num
  end_tran()

  table.insert(lst, num)

  for i, n in ipairs(lst) do
    io.write(n, " ")
  end
  io.write("\n")
end
