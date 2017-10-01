PYTHON_FILE=detect_system.py
TEMP_FILE=tempfile
SYSTEM=`python $(PYTHON_FILE)`
SYSTEM=`cat $(TEMP_FILE)`
all:	
	@echo $(SYSTEM)
	./load_modules.sh	
	qsub -l nodes=8:ppn=16 submit_script.sh