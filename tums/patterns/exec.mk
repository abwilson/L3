$(execs): %: | $(mkdir)
	$(LINK.cpp) -o $@ $^ 
