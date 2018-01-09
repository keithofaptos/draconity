import bson
import os
import zmq
base = os.path.expanduser('~/.talon/.sys')

def multrecv(s):
    ret = [s.recv()]
    while s.getsockopt(zmq.RCVMORE):
        ret.append(s.recv())
    return ret

context = zmq.Context()
s = context.socket(zmq.REQ)
s.connect('ipc://%s/dc_cmd.sock' % base)

def call(d):
    s.send(bson.BSON.encode(d))
    return bson.decode_all(s.recv())[0]

def listener():
    s = context.socket(zmq.SUB)
    s.setsockopt(zmq.SUBSCRIBE, '')
    s.setsockopt(zmq.TCP_KEEPALIVE, 1)
    s.setsockopt(zmq.TCP_KEEPALIVE_IDLE, 3000)
    s.setsockopt(zmq.TCP_KEEPALIVE_INTVL, 1000)
    s.connect('ipc://%s/dc_pub.sock' % base)
    return s

def wait():
    if call({'cmd': 'status'})['ready']:
        return

    l = listener()
    while True:
        topic, data = multrecv(l)
        if topic == 'status':
            data = bson.decode_all(data)[0]
            if data.get('cmd') == 'ready':
                break
    l.close()
