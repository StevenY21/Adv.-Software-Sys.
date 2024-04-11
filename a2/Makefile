all: libtap.so tappet 
observe.o: observe.c
		gcc -fPIC -c observe.c
reconstruct.o: reconstruct.c
		gcc -fPIC -c reconstruct.c
libtap.so: observe.o
		gcc -shared -o libtap.so observe.o reconstruct.o
tappet: tappet.c
		gcc tappet.c -o tappet -L. -ltap