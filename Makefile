serv.out: C_ANS.o C_API.o cJSON.o C_SERVER.o C_DB.o
	gcc -o serv.out C_DB.o C_ANS.o C_API.o cJSON.o C_SERVER.o -lcurl -lm -lsqlite3

C_ANS.o: C_ANS.c
	gcc -c -o C_ANS.o C_ANS.c

C_API.o: C_API.c
	gcc -c -o C_API.o C_API.c

cJSON.o: cJSON.c
	gcc -c -o cJSON.o cJSON.c

C_SERVER.o: C_SERVER.c
	gcc -c -o C_SERVER.o C_SERVER.c

C_DB.o: C_DB.c
	gcc -c -o C_DB.o C_DB.c

clean:
	rm *.o serv.out

