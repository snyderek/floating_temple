# Sum the integers from 1 to 10 on a single machine, using two threads.

import threading

class Summer:
  def __init__(self, total):
    self.__total = total
    self.__lock = threading.Lock()

  def add(self, increment):
    self.__lock.acquire()
    self.__total += increment
    print(increment, self.__total)
    self.__lock.release()

  def total(self):
    self.__lock.acquire()
    temp = self.__total
    self.__lock.release()
    return temp

def thread_func(summer, start):
  for i in range(start, 11, 2):
    summer.add(i)

summer = Summer(0)

child_thread = threading.Thread(target = thread_func, args = (summer, 2))
child_thread.start()
thread_func(summer, 1)
child_thread.join()

print()
print("Total:", summer.total())
