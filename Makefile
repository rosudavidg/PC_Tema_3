build:
	gcc -o dnsclient dnsclient.c

run:
	./dnsclient google.com A

clean:
	rm -f dnsclient message dns.log message.log
