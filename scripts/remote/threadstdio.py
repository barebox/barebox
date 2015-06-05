#!/usr/bin/python2

import os
import sys
import termios
import atexit
from threading import Thread
from Queue import Queue, Empty

class ConsoleInput(Thread):
    def __init__(self, queue, exit='\x14'):
        Thread.__init__(self)
        self.daemon = True
        self.q = queue
        self._exit = exit
        self.fd = sys.stdin.fileno()
        old = termios.tcgetattr(self.fd)
        new = termios.tcgetattr(self.fd)
        new[3] = new[3] & ~termios.ICANON & ~termios.ECHO & ~termios.ISIG
        new[6][termios.VMIN] = 1
        new[6][termios.VTIME] = 0
        termios.tcsetattr(self.fd, termios.TCSANOW, new)

        def cleanup():
            termios.tcsetattr(self.fd, termios.TCSAFLUSH, old)
        atexit.register(cleanup)

    def run(self):
        while True:
            c = os.read(self.fd, 1)
            if c == self._exit:
                self.q.put((self, None))
                return
            else:
                self.q.put((self, c))

if __name__ == "__main__":
    q = Queue()
    i = ConsoleInput(q)
    i.start()
    while True:
        event = q.get(block=True)
        src, c = event
        if c == '\x04':
            break
        os.write(sys.stdout.fileno(), c.upper())

