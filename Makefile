# ----------------------------------------------------------------------------#
# --------------- MAKEFILE FOR DELTAMODELS -----------------------------------#
# ----------------------------------------------------------------------------#


# --------------- DECLARATIONS -----------------------------------------------#

.DEFAULT_GOAL := help
.PHONY: help
help: ## List of main goals
	@awk ' BEGIN {FS = ":.*?## "} /^[a-zA-Z_-]+:.*?## / \
	{printf "\033[36m%-30s\033[0m %s\n", $$1, $$2}' $(MAKEFILE_LIST)

ifeq ($(OS),Windows_NT)
  # Windows is not supported!
else
  # Some commands are different in Linux and Mac
  UNAME_S := $(shell uname -s)

  # User's credential will be passed to the image and container
  USERNAME=$(shell whoami)
  USER_UID=$(shell id -u)
  USER_GID=$(shell id -g)
endif

PWD=$(shell pwd)
WPWD = $(shell cygpath -w ${PWD})

IMAGENAME=deltamodels
CONTAINERNAME=deltamodels
ENV_HASH=$(shell cat environment/Dockerfile environment/apt-list.txt | shasum -a 1 | tr " " "\n" | head -n 1)

DEXEC = docker exec \
	--interactive \
	--tty \
	$(shell cat container)

DEXECJENKINS = docker exec $(shell cat container)

BUILD_THREADS := $(shell cat /proc/cpuinfo | grep processor | wc -l)

ifeq ($(shell uname -s),Darwin)
HOST=OSX
else
ifneq (,$(findstring NT,$(shell uname -s)))
HOST=windows
else
HOST=linuxX11
endif
endif

# --------------- DOCKER STUFF -----------------------------------------------#

.PHONY: build
build: environment/Dockerfile
	docker build . \
	--file environment/Dockerfile \
	--tag ${IMAGENAME} \
	--build-arg USERNAME=${USERNAME} \
	--build-arg USER_UID=${USER_UID} \
	--build-arg USER_GID=${USER_GID} \

container:
	make build
	docker run \
	--rm \
	-dt \
	-v `pwd`:/workdir \
	-w /workdir \
	--name=${CONTAINERNAME} \
	--privileged \
	--cidfile=container \
	--net=host --ipc=host \
	${IMAGENAME} /bin/bash
	mkdir -p build


.PHONY: shell
shell: container
	docker exec -it $(shell cat container) /bin/bash

CPPFLAGS=-std=c++17 \
	-DSC_CPLUSPLUS=201703L \
	-D_FORTIFY_SOURCE=2 \
	-faligned-new \
	-fstack-protector \
	-Wall \
	-Werror \
	-g \
	-grecord-gcc-switches \
	-O2 \
	-DDEBUG
SYSC_INC = -I$(shell ${DEXEC} printenv SYSTEMC_INCLUDE)
TBB_INC = -I$(shell ${DEXEC} printenv TBB_INCLUDE)
PYINCLUDES = $(shell ${DEXEC} python3.8-config --cflags)
VERILATOR_INC = -I/usr/local/share/verilator/include/
INCLUDE = ${SYSC_INC} ${TBB_INC} ${PYINCLUDES} ${VERILATOR_INC} -I./
SYSC_LNK = -L$(shell ${DEXEC} printenv SYSTEMC_HOME)/lib-linux64/
TBB_LNK = -L$(shell ${DEXEC} printenv TBB_LIBRARY_RELEASE)
PYLINK=/usr/local/lib/python3.8/config-3.8-x86_64-linux-gnu/libpython3.8.a \
	$(shell ${DEXEC} python3.8-config --ldflags)

# add sub-makefiles to keep the build logic or examples and tests somewhat confined

.PHONY: drun
drun: container
	${DEXEC} $(RUN_ARGS)

DRUNBUILD= docker exec \
	-w /workdir/build \
	$(shell cat container)


build/CMakeCache.txt: container
	${DRUNBUILD} cmake .. ; \
	${DRUNBUILD} make       

# --------------- DOCUMENTATION ----------------------------------------------#

.PHONY: docs-internal
docs-internal: container ## Proper docs
	-${DEXECJENKINS} rm -rf /docs-internal
	-${DEXECJENKINS} doxygen config.doxy 

.PHONY: docs
docs: container ## Public facing docs
	${DEXECJENKINS} make -C docs all

.PHONY: docs-html
docs-html: container ## Public facing docs
	${DEXECJENKINS} make -C docs html

.PHONY: docs-pdf
docs-pdf: container ## Public facing docs
	${DEXECJENKINS} make -C docs pdf

.PHONY: docs-epub
docs-epub: container ## Public facing docs
	${DEXECJENKINS} make -C docs epub

# --------------- TESTING ----------------------------------------------------#

# Terrible work-around for copying with failure on setting CAP_RAW on a volume.
# It needs to be fixed.
.PHONY: tests
tests: build/CMakeCache.txt
	-${DRUNBUILD} sudo ctest -T Test --no-compress-output --output-on-failure  

COVERAGE_FLAGS="--gcov-exclude='.*logging.*'"
.PHONY: coverage
coverage:
	-${DRUNBUILD} gcovr -r /workdir  
	-${DRUNBUILD} gcovr -r /workdir --xml-pretty  -o /workdir/coverage.xml

.PHONY: memcheck
memcheck:
	-${DRUNBUILD} sudo ctest -D ExperimentalMemCheck

.PHONY: cppcheck
cppcheck: container
	-${DEXECJENKINS} cppcheck \
	--enable=warning,style,performance,portability \
	--inconclusive \
	--xml \
	--xml-version=2 \
	-I. -I $SYSTEMC_INCLUDE models tlms utils 2> cppcheck.xml

# --------------- QA ---------------------------------------------------------#


# --------------- PACKAGING --------------------------------------------------#


# --------------- CLEANING ---------------------------------------------------#

.PHONY: clean
clean: clean-docs clean-logs clean-container
	-rm -rf build/
	-rm -f *.vcd
	-find . -type f -name "*.o" -exec rm -f {} \;

.ONESHELL:
.PHONY: clean-container
clean-container: ## Stop the container
	docker ps -q --filter "name=${CONTAINERNAME}" | grep -q . && \
	docker stop ${CONTAINERNAME} || true
	rm -f container

.PHONY: clean-docs
clean-docs:
	rm -rf docs/_build docs-internal

.PHONY: clean-logs
clean-logs:
	rm -f docs/sphinx-build-*.log

# --------------- DEVELOPMENT ------------------------------------------------#
