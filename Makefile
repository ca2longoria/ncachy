
all:
	gcc -O2 ccachy.c -o ccachy

debug:
	gcc -O2 ccachy.c -o ccachy -D DEBUG
	

