include		Makefile.defs

ALL_COMPONENTS		= binfmt-coff binfmt-elf binfmt-xout ibcs per-cxenix per-isc per-sco per-solaris per-svr4 per-uw7 per-wyse 
ALL_COMPONENT_OBJS	= $(foreach component, $(ALL_COMPONENTS), $(BUILD_DIR)/$(component).o)



all: $(BUILD_DIR)/ibcs-us

$(BUILD_DIR)/ibcs-us: $(ALL_COMPONENT_OBJS) Makefile
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(ALL_COMPONENT_OBJS)

$(ALL_COMPONENT_OBJS):	FORCE
	$(MAKE) --directory $(patsubst %.o, %, $(@F)) ../$@

#
# Just give me the compilr command lines.
#
.PHONY: cc
cc:
	@echo $(CC) $(CFLAGS)

.PHONY: ld
ld:
	@echo $(CC) $(CFLAGS) $(LDFLAGS)

#
# Include files are supposed to include everything they need.  This does
# an incomplete test of that, incomplete because it doesn't see into macros.
#
.PHONY:	test-includes
test-includes:
	mkdir -p $(BUILD_DIR)
	set -e; \
	    target="$(strip $(BUILD_DIR))/test-include"; \
	    for f in $$(cd include; find . -name "*.h" | sort); do \
	        printf "\tTesting %s\n" "$${f}"; \
		printf "#include <%s>\n" "$${f}" > "$${target}.c"; \
		$(CC) $(CFLAGS) -E -o $${target}.i $${target}.c; \
		$(CC) $(CFLAGS) -c -o $${target}.o $${target}.c; \
	    done; \
	    rm $${target}.[cio]

clean::
	for dir in $(ALL_COMPONENTS); do $(MAKE) --directory $$dir "$@"; done
	rm -fr $(BUILD_DIR)

distclean: clean
	find $(ROOT_DIR) -path ./.hg -prune -o \( -name gdb_history -o -name ".*.sw?" -o -name "*.[iso]" \) -print | xargs -d '\n' -r rm -f

#
# Link all the unit tests together and run them.
#
# Files with unit tests have "#ifdef UNIT_TEST" in them.  When -DUNIT_TEST
# is in effect they define a function called unit_test_COMPONENT_FILENAME_c(),
# and when you call it a unit test is done.
#
.PHONY:	tests
TEST_SRCS	:= $(shell $(UNIT_TEST_GREP) $(sort $(foreach dir, $(ALL_COMPONENTS), $(dir)/*.c)))
unit-tests: $(BUILD_DIR)/tests/unit-tests
	$(BUILD_DIR)/tests/unit-tests

$(BUILD_TEST_DIR)/unit-tests: $(foreach comp, $(sort $(foreach src, $(TEST_SRCS), $(dir $(src)))), $(BUILD_TEST_DIR)/$(comp:%/=%).o)
	( \
	  printf "extern void unit_test_%s_%s_c(int ac, const char** av, const char** ep);\n" $(foreach src, $(TEST_SRCS), $(patsubst %/, %, $(dir $(src))) $(subst -,_,$(notdir $(src:.c=)))); \
	  printf "\nint main(int ac, const char** av, const char** ep)\n{\n"; \
	  printf "    unit_test_%s_%s_c(ac, av, ep);\n" $(foreach src, $(TEST_SRCS), $(patsubst %/, %, $(dir $(src)) $(subst -,_,$(notdir $(src:.c=))))); \
	  printf "    return 0;\n}\n"; \
	) > $(BUILD_TEST_DIR)/unit-tests.c
	$(CC) $(CFLAGS) -Og -g -o $(BUILD_TEST_DIR)/unit-tests $(BUILD_TEST_DIR)/unit-tests.c $^


$(BUILD_TEST_DIR)/%.o: FORCE
	$(MAKE) --directory $(@F:.o=) ../$@

#
# Boilerplate.
#
.PHONY: FORCE
FORCE:

include Makefile.release
