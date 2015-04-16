from peer import begin_tran, end_tran, shared

begin_tran()
num = shared.setdefault('num', 0) + 1
end_tran()

print('My unique number is', num)
