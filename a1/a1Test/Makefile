all: libobjdata.a getsections getsyms getcopy libobjdata.so getsections_dl getsyms_dl getcopy_dl

objsecta.o: objsect.c
		gcc -c objsect.c -o objsecta.o
objsyma.o: objsym.c
		gcc -c objsym.c -o objsyma.o
objcopya.o: objcopy.c
		gcc -c objcopy.c -o objcopya.o
libobjdata.a: objsecta.o objsyma.o objcopya.o
		ar rc libobjdata.a objsecta.o objsyma.o objcopya.o
objsect.o: objsect.c
		gcc -fpic -c objsect.c
objsym.o: objsym.c
		gcc -fpic -c objsym.c
objcopy.o: objcopy.c
		gcc -fpic -c objcopy.c
libobjdata.so: objsect.o objsym.o objcopy.o
		gcc -shared -o libobjdata.so objsect.o objsym.o objcopy.o -lbfd
getsections: getsections.c
		gcc getsections.c -o getsections -lbfd -lm -L. -lobjdata 
getsyms: getsyms.c
		gcc getsyms.c -o getsyms -lbfd -lm -L. -lobjdata 
getcopy: getcopy.c
		gcc getcopy.c -o getcopy -L. -lobjdata -lbfd
getsections_dl: getsections_dl.c
		gcc getsections_dl.c -o getsections_dl -lbfd -lm -L. -lobjdata 
getsyms_dl: getsyms_dl.c
		gcc getsyms_dl.c -o getsyms_dl -lbfd -lm -L. -lobjdata 
getcopy_dl: getcopy_dl.c
		gcc getcopy_dl.c -o getcopy_dl -lbfd -lm -L. -lobjdata 