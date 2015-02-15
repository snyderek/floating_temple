# Sum the integers from 1 to 10 on two peers, using the distributed interpreter.

import peer
import time

class Summer:
  def __init__(self, total):
    self.__total = total

  def add(self, increment):
    self.__total += increment
    print(increment, self.__total)

  def total(self):
    return self.__total

def thread_func(summer, start):
  for i in range(start, 11, 2):
    summer.add(i)

summer = Summer(0)

peer.CreateThread(thread_func, (summer, 2))
thread_func(summer, 1)

# TODO(dss): Instead of sleeping for an arbitrary number of seconds, detect when
# the thread on the remote peer has completed.
time.sleep(10)
