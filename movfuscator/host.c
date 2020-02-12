#include <string.h>

#ifndef LCCDIR
#define LCCDIR "/usr/local/lib/lcc/"
#endif

#define CRT0 LCCDIR "/crt0.o"
#define CRTF LCCDIR "/crtf.o"
#define CRTD LCCDIR "/crtd.o"

#define CRT0_CF LCCDIR "/crt0_cf.o"
#define CRTF_CF LCCDIR "/crtf_cf.o"
#define CRTD_CF LCCDIR "/crtd_cf.o"

char *suffixes[]={
	".c",
	".i",
	".s",
	".o",
	".out",
	0
};

char inputs[256]="";

char *cpp[]={
	"/usr/bin/cpp",
	"-U__GNUC__",
	"-D_POSIX_SOURCE",
	/* "-D__STDC__=1", */
	"-D__STRICT_ANSI__",
	"-Dunix",
	"-Di386",
	"-Dlinux",
	"-D__unix__",
	"-D__i386__", 
	"-D__linux__", 
	"-D__signed__=signed",
	"-std=gnu90",
	"$1",
	"$2",
	"$3",
	0 
};

char *include[]={
	"-I" LCCDIR "include", 
	"-I" LCCDIR "gcc/include",
	"-I/usr/include",
	0 
};

char *com[]={
	LCCDIR "rcc",
	"-target=x86/mov",
	"$1",
	"$2",
	"$3",
	0
};

char *as[]={
	"/usr/bin/as",
	"--32",
	"-o",
	"$3",
	"$1",
	"$2",
	0
};

char *ld[]={
	"/usr/bin/ld", 
	"-m",
	"elf_i386", 
	"-dynamic-linker",
	"/lib/ld-linux.so.2", 
	"-L" LCCDIR,
	"-L" LCCDIR "/gcc/32",
	"-lgcc",
	"-lc",
	"-lm",
	CRT0,
	"$1",
	"$2",
	CRTF,
	CRTD,
	"-o",
	"$3",
	0
};

int option(char* arg) 
{
	if (strcmp(arg, "-g")==0) {
		return 1;
	}
	else if (strcmp(arg, "--no-mov-flow")==0) {
		/* compiling with --no-mov-flow requires linking against the crt
		 * libraries also compiled with --no-mov-flow */
		int i=0;
		while (ld[i]) {
			if (strcmp(ld[i], CRT0)==0) {
				ld[i]=CRT0_CF;
			}
			else if (strcmp(ld[i], CRTF)==0) {
				ld[i]=CRTF_CF;
			}
			else if (strcmp(ld[i], CRTD)==0) {
				ld[i]=CRTD_CF;
			}
			i++;
		}
		return 1;
	}
	else {
		return 0;
	}
}
