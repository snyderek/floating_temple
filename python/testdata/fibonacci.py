# Fibonacci sequence

a = 0
b = 1

for i in range(20):
  print(a)
  temp = a
  a = b
  b += temp
