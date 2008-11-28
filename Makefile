# Compatibility for us old-timers.
PHONY=check clean dist test
all: check
dist: 
	python ./setup.py sdist bdist
check: 
	nosetests
clean: 
	python ./setup.py $@
test: check
.PHONY: $(PHONY)
