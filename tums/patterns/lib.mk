$(bld)/lib%.a: | $(mkdir)
	ar -r $@ $^
