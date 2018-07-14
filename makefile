all: 
	-cd coordinador && $(MAKE) all
	-cd planificador && $(MAKE) all
	-cd instanica && $(MAKE) all
	-cd esi && $(MAKE) all

clean:
	-cd coordinador && $(MAKE) clean
	-cd planificador && $(MAKE) clean
	-cd instancia && $(MAKE) clean
	-cd esi && $(MAKE) clean