from peer import begin_tran, end_tran, shared

begin_tran()
lst = shared.setdefault('lst', [0, 1])
end_tran()

while True:
  begin_tran()
  next_num = lst[-2] + lst[-1]
  s.append(next_num)
  end_tran()

  print(lst[-10:])
