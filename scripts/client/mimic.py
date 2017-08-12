from common import call, wait
import sys
wait()
print call({'cmd': 'mimic', 'phrase': sys.argv[1:]})
