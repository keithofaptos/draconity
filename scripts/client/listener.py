from common import listener, multrecv
import bson
import threading
import time

s = listener()

print 'sub listening'
start = 0
last = 0
while True:
    topic, cmd = multrecv(s)
    cmd = bson.decode_all(cmd)[0]
    diff = 0
    period = 0
    if topic == 'phrase' and cmd['cmd'] in ('p.begin', 'p.end'):
        if cmd['cmd'] == 'p.begin':
            if last:
                period = time.time() - last
            last = start = time.time()
        elif cmd['cmd'] == 'p.end':
            diff = time.time() - start
        print 'sub <- (%.2fms %.2fms %s %s)' % (period * 1000, diff * 1000, topic, cmd)
    else:
        print 'sub <- %s, %s' % (topic, cmd)
