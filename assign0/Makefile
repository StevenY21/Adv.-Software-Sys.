all: objsecta.o objsyma.o libobjdata.a getsections getsyms objsect.o objsym.o libobjdata.so getsections_dl getsyms_dl

objsecta.o: objsect.c
		gcc -c objsect.c -o objsecta.o
objsyma.o: objsym.c
		gcc -c objsym.c -o objsyma.o
libobjdata.a: objsecta.o objsyma.o
		ar rc libobjdata.a objsecta.o objsyma.o
getsections: getsections.c
		gcc getsections.c -o getsections -lbfd -lm -L. -lobjdata 
getsyms: getsyms.c
		gcc getsyms.c -o getsyms -lbfd -lm -L. -lobjdata 
# NOTE might need to use cc here for some strange reason or export library path later
objsect.o: objsect.c
		gcc -fpic -c objsect.c
objsym.o: objsym.c
		gcc -fpic -c objsym.c
libobjdata.so: objsect.o objsym.o
		gcc -shared -o libobjdata.so objsect.o objsym.o
getsections_dl: getsections_dl.c
		gcc getsections_dl.c -o getsections_dl -lbfd -lm -L. -lobjdata 
getsyms_dl: getsyms_dl.c
		gcc getsyms_dl.c -o getsyms_dl -lbfd -lm -L. -lobjdata 