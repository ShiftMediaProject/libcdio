# Compatibility for us old-timers.
PHONY=check test dist
all: check
dist: 
	python ./setup.py sdist bdist
check: 
	nosetests
test: check
.PHONY: $(PHONY)
