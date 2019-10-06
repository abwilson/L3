.SECONDEXPANSION:
$(bld_root)/%.o: %.cpp $$(cpp_read_deps) | $(mkdir)
	$(COMPILE.cpp) $< -o $@

#	$(warning deps: $^)
