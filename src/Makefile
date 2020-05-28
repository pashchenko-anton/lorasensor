lorasensor: radio.o LoRaBase.o
	$(CC) $(CFLAGS) radio.o LoRaBase.o $(LIBS) -lrt -ldl lorasensor.c -o lorasensor

radio.o: radio.c
	$(CC) $(CFLAGS) -c radio.c

LoRaBase.o: LoRaBase.c
	$(CC) $(CFLAGS) -c LoRaBase.c

clean:
	rm *.o my_lora
