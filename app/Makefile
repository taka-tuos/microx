MAKE_APPS = find ./ -maxdepth 1 -type d | cut -d/ -f2 | xargs -I{} make -C {}

default : 
	$(MAKE_APPS)

clean :
	$(MAKE_APPS) clean

