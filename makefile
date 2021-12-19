PROJECTS=kernel use-case-deadlock use-case-scheduling use-case-suspension tests
LIBS=static matelib

all: $(PROJECTS)
	@echo "Makefile para rama \"$(GIT_BRANCH)\". Se compilaron \"$(PROJECTS)\""

$(PROJECTS): $(LIBS)
	$(MAKE) --no-print-directory -C $@ all

$(LIBS):
	$(MAKE) --no-print-directory -C $@ all

clean: cleanvalgrindfiles cleanlogs
	$(foreach P, $(LIBS) $(PROJECTS), $(MAKE) --no-print-directory -C $P clean;)

cleanvalgrindfiles:
	$(RM) **/vgcore.*

cleanlogs:
	$(RM) **/**/*.log

release:
	$(foreach P, $(LIBS) $(PROJECTS), $(MAKE) --no-print-directory -C $P release;)

.PHONY: all $(PROJECTS) $(LIBS) clean release cleanvalgrindfiles cleanlogs
