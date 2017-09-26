import platform
import os
import sys

COMPILED_FILE = 'output'

if len(sys.argv) < 3:
    print('input format : detect_system.py [filename] [cluster/local] [options]')
    raise SystemExit(0)

filename = sys.argv[1]
options = ''

def run_process(command):
    print(command)
    os.system(command)

if len(sys.argv) > 3:
    for element in sys.argv[3:]:
        options += ' ' + element

if len(sys.argv) > 2 and sys.argv[1] == 'cluster':
    # RUN ON CLUSTER
    run_process('./load_modules.sh') # load modules
    run_process('mpicc -g -Wall -o ' + COMPILED_FILE + ' ' + filename) # compile
    run_process('chmod 777 ./' + COMPILED_FILE)
    run_process('qsub' + options + ' ./' + COMPILED_FILE) # queue
else:
    # RUN LOCALLY
    run_process('mpicc -g -Wall -o ' + COMPILED_FILE + ' ' + filename) # compile 
    run_process('chmod 777 ./' + COMPILED_FILE)
    run_process('mpiexec' + options + ' ./' + COMPILED_FILE)
