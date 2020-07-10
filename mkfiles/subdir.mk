MAKE_SUBDIR = find ./ -maxdepth 1 -type d | cut -d/ -f2 | xargs -I{} make -C {}

default : 
	$(MAKE_SUBDIR)

clean :
	$(MAKE_SUBDIR) clean
