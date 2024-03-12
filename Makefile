export TOPDIR = $(PWD)
export TOPTARGETS := all clean check

SUBDIRS := lib thermal-engine

$(TOPTARGETS): $(SUBDIRS)

$(SUBDIRS):
	@$(MAKE) -C $@ $(MAKECMDGOALS)

.PHONY: $(TOPTARGETS) $(SUBDIRS)
