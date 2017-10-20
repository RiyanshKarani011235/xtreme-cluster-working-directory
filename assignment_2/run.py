import platform
import os
import sys

COMPILED_FILE = './output'
SUBMIT_SCRIPT = './submit_script.sh'
LOG_FILE = './output.log'

try:
    os.system('rm ' + LOG_FILE)
except:
    pass

if len(sys.argv) < 3:
    print('input format : detect_system.py [filename] [cluster/local] [options]')
    raise SystemExit(0)

filename = sys.argv[1]
options = ''

new_submit_script = ''
with open(SUBMIT_SCRIPT, 'r') as f:
    for line in f:
        if 'mpirun' in line:
            new_line = ''
            for element in line.strip().split(' ')[:-1]:
                new_line += element + ' '
            new_line += COMPILED_FILE
            new_submit_script += new_line + '\n'
        else:
            new_submit_script += line
    f.close()
with open(SUBMIT_SCRIPT, 'w') as f:
    f.write(new_submit_script)
    f.close()

def run_process(command):
    print(command)
    os.system(command)

if len(sys.argv) > 3:
    for element in sys.argv[3:]:
        options += ' ' + element

if len(sys.argv) > 2 and sys.argv[2] == 'cluster':
    # RUN ON CLUSTER
    run_process('./load_modules.sh') # load modules
    run_process('mpicc -o ' + COMPILED_FILE + ' ' + filename + ' -std=c99') # compile
    run_process('chmod 777 ' + COMPILED_FILE)
    run_process('qsub' + options + ' ' + SUBMIT_SCRIPT) # queue
else:
    # RUN LOCALLY
    run_process('mpicc -g -Wall -o ' + COMPILED_FILE + ' ' + filename) # compile 
    run_process('chmod 777 ' + COMPILED_FILE)
    run_process('mpiexec' + options + ' ' + COMPILED_FILE)

os.system('vim output.log')