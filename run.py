import sys
import os

if __name__ == '__main__':
    args = ' '.join(sys.argv[1:])
    os.system('./scheduler ' + args)
    # print('./scheduler ' + args)
    os.system('/home/frankeh/Public/sched ' + args)
    # print('/home/frankeh/Public/sched ' + args)