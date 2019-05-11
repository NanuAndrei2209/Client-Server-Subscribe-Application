323CA
Nanu Andrei

Data inceperii: 26 aprilie 2019
Data finalizarii: 6 mai 2019

Usage:
	make -> make run_server -> ./subscriber name IPserver port(how many you want) -> subcribe to a topic by introducing 
	command subscribe topic_name sf(store-and-forward 1 = activate 0 = inactive) ->
	python3 udp_client.py 127.*.*.* (anything but 1 255 for the last one) port(> 1024)
	--input_file three_topics_payloads.json --source-address 0.0.0.0 --source-port (something unused)

	- then the udp clients will send to server information about topics, so the clients that subscribed on that topic
	will receive the information

Client:

	- se conecteaza la server prin adresa IP si port-ul date ca argument executabilului
	
	- are implementat comanda exit

	- inafara comenzii exit, poate primi doar comenzi de genul 1) subscribe topic sf sau 2) unsubscribe topic
	orice alta comanda fiind evaluata ca gresita

	- in momentul primirii unei comenzi de subscribe/unsubscribe, acesta doar trimite mesajul la server,
	care se ocupa de acesta

Server:

	- un socket tcp pe care se face listen pentru a se conecta cu clientii

	- un socket udp pe care asteapta mesajele

	- cand e activ socketul de listen, inseamna ca s-a conectat un client, il adaugam in vectorul de clienti
	sau verificam daca a fost adaugat inainte si il marcam ca online

	- cand e activ socketul de "stdin", adica cel cu file descriptorul 0, inseamna ca primim o comanda de la
	tastatura, iar aceasta poate fi doar exit, caz in care se vor inchide socketii si se va opri programul

	- cand e activ socketul de udp, inseamna ca am primit un mesaj, pe care il receptionam, il transformam
	a.i. clientul sa trebuiasca doar sa il afiseze si il trimitem (mult mai multe detalii in comentariile de cod)

	- cand e activ socketul unui client, inseamna ca am primit o comanda de tipul subscribe/unsubscribe,
	si vom realiza modificarile corespunzatoare clientului respectiv din vectorul de clienti
	(mult mai multe detalii in comentariile de cod x 2)
Classes.h:
	- clasa Topic cu 2 campuri : string si int, reprezentand topicul si store and forward-ul
	- clasa Client cu : nume, vector de topicuri, vector de mesaje(inbox), file descriptorul socketului prin care
	este conectat si un bool connected care spune daca acesta este conectat la un moment dat sau nu
