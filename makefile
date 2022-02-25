PROJECTS=kernel use-case-deadlock use-case-scheduling use-case-suspension tests
LIBS=static matelib

all: $(PROJECTS)
	@echo "Proyectos compilados: \"$(PROJECTS)\""
	@echo "Dependencias compiladas: \"$(LIBS)\""

$(PROJECTS): $(LIBS)
	$(MAKE) --no-print-directory -C $@ all

$(LIBS):
	$(MAKE) --no-print-directory -C $@ all

clean: clean-vgcores clean-logs
	$(foreach P, $(LIBS) $(PROJECTS), $(MAKE) --no-print-directory -C $P clean;)

clean-vgcores:
	$(RM) **/vgcore.*

clean-logs:
	$(RM) **/**/*.log

release:
	$(foreach P, $(LIBS) $(PROJECTS), $(MAKE) --no-print-directory -C $P release;)

.PHONY: all $(PROJECTS) $(LIBS) clean release clean-vgcores clean-logs
