# The top level Makefile

SRC_DIR=src

all:
	cd ${SRC_DIR}/animate 	&& $(MAKE)
	cd ${SRC_DIR}/misc      && $(MAKE)
	cd ${SRC_DIR}/unicode	&& $(MAKE)
	cd ${SRC_DIR}/windows	&& $(MAKE)
	cd ${SRC_DIR}/gookin	&& $(MAKE)
	cd ${SRC_DIR}/color	    && $(MAKE)
	cd ${SRC_DIR}/mazes	    && $(MAKE)


	@echo
	@echo "*********************************************"
	@echo "All files Built"
	@echo "Please move to ../out directory"
	@echo "Execute each file to see examples in action"
	@echo "*********************************************"
	@echo
clean:
	cd ${SRC_DIR}/animate  && $(MAKE) clean
	cd ${SRC_DIR}/misc     && $(MAKE) clean
	cd ${SRC_DIR}/unicode  && $(MAKE) clean
	cd ${SRC_DIR}/windows  && $(MAKE) clean
	cd ${SRC_DIR}/gookin   && $(MAKE) clean
	cd ${SRC_DIR}/color	   && $(MAKE) clean
	cd ${SRC_DIR}/mazes	    && $(MAKE) clean
