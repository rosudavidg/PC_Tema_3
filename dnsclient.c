/*
	Rosu Gabriel - David
	321CD
	Tema 3 - PC
	22 mai 2018
*/

/*
	Exit codes:
	0 - succes
	1 - numar de argumente incorect
	2 - comanda incorecta (cel de-al doilea argument)
	3 - mesajul nu a putut fi transmis
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>

#include "dnsclient.h"

#define FILE_NAME_DNS_SERVERS "dns_servers.conf"
#define FILE_NAME_DNS_LOG     "dns.log"
#define FILE_NAME_MESSAGE_LOG "message.log"
#define MAX_DNS_SERVERS_NO 100            // Numarul maxim de servere DNS
#define DNS_PORT 53                       // PORTUL DNS
#define DNS_MESSAGE_SIZE 65535            // MAX SIZE (RFC)

char dns_servers[MAX_DNS_SERVERS_NO][16]; // serverele DNS
int  dns_servers_no;                      // numarul de servere DNS

int type;                                 // tipul interogarii
dns_header_t dns_header;                  // headerul DNS
char qname[256];                          // numele adresei pentru interogare
struct sockaddr_in to_station;            // serverul dns catre care trimit mesajul
int sockfd;                               // socket file descriptor
unsigned char message[DNS_MESSAGE_SIZE];  // zona de memorie pentru trimiterea/ primirea unui mesaj
int selected_DNS;                         // conexiunea care functioneaza

// char qname_ans[256];                   // primit ca raspuns
char name_ans[256];                       // primit ca raspuns

// Verific daca numarul de argumente este exact 2.
void check_argc(int argc) {
	
	if (argc < 3) {
		exit(1);
	} else if (argc > 3) {
		exit(1);
	}
}

// Citesc serverele DNS din fisier
void read_dns_servers() {

    FILE* fd;
    char* line = NULL;
    size_t len = 0;
    ssize_t read;

    fd = fopen(FILE_NAME_DNS_SERVERS, "rt");

    while ((read = getline(&line, &len, fd)) != -1) {

    	if (line[0] != '#') {
    		dns_servers_no++;

    		for (int i = 0; i < read - 1; i++) {
    			dns_servers[dns_servers_no][i] = line[i];
    		}
    	}
    }

    fclose(fd);
}

// Transformare tip din string in int
void find_type(char *type_char) {
	
	if (strcmp(type_char, "A") == 0) {
		type = A;
	} else if (strcmp(type_char, "NS") == 0) {
		type = NS;
	} else if (strcmp(type_char, "CNAME") == 0) {
		type = CNAME;
	} else if (strcmp(type_char, "MX") == 0) {
		type = MX;
	} else if (strcmp(type_char, "SOA") == 0) {
		type = SOA;
	} else if (strcmp(type_char, "TXT") == 0) {
		type = TXT;
	} else if (strcmp(type_char, "PTR") == 0) {
		type = PTR;
	} else {
		exit(2);
	}
}

// Transform din name in qname
void set_qname(char* string, int len) {

	int pos  = 0;
	int size = 0;
	int i    = 1;
	int dot  = 0;

	while (pos < len) {

		if (string[pos] == '.') {
			qname[dot] = size;
			size = 0;
			dot = i;
			i++;
		} else {
			qname[i] = string[pos];
			size++;
			i++;
		}

		pos++;
	}

	qname[dot] = size;
	qname[i] = '\0';
}

// Initializare socket UDP
void init_socket() {

    sockfd = socket(AF_INET , SOCK_DGRAM , 0);

    struct timeval tv;
	tv.tv_sec  = 2;
	tv.tv_usec = 0;
	setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
}

// Initializare statie pentru trimiterea mesajului
void init_to_station(char* dns_server) {

	to_station.sin_family      = AF_INET;
    to_station.sin_port        = htons(DNS_PORT);
    to_station.sin_addr.s_addr = inet_addr(dns_server);
}

int send_message() {
	if (sendto(sockfd, message, sizeof(dns_header_t) + (strlen(qname) + 1) + sizeof(dns_question_t), 0, (struct sockaddr*) &to_station, sizeof(to_station)) < 0) {
        return 1;
    }

    return 0;
}

// Incarca mesajul in zona de memorie alocata variabilei message
void compress_query() {

	// Crearea Header-ului
	dns_header_t* dns_header_ptr = (dns_header_t *) &message;

	dns_header_ptr->id      = (getpid() % 2 == 0 ? htons(0xdead) : htons(0xbeef));

    dns_header_ptr->rd      = 1;
    dns_header_ptr->tc      = 0;
    dns_header_ptr->aa      = 0;
    dns_header_ptr->opcode  = 0;
	dns_header_ptr->qr      = 0;

    dns_header_ptr->rcode   = 0;
    dns_header_ptr->z       = 0;
    dns_header_ptr->ra      = 0;

    dns_header_ptr->qdcount = 0x0100; // 1
    dns_header_ptr->ancount = 0;
    dns_header_ptr->nscount = 0;
    dns_header_ptr->arcount = 0;

    // Crearea numelui
    char* qname_ptr = (char *) &message[sizeof(dns_header_t)];

    for (int i = 0; i < strlen(qname); i++) {
    	qname_ptr[i] = qname[i];
    }

    qname_ptr[strlen(qname)] = 0x00;

    // Crearea intrebarii
    dns_question_t* dns_question_ptr = (dns_question_t *) &message[sizeof(dns_header_t) + (strlen(qname) + 1)];

    dns_question_ptr->qtype  = htons(type);
    dns_question_ptr->qclass = htons(1);
}

void send_and_receive_query() {

	int size_sock = sizeof(to_station);
	int attempt = 1;
	init_to_station(dns_servers[attempt]);

	while (1) {
		if (send_message() != 0) {
			
			attempt++;
			
			if (attempt > dns_servers_no) {
				exit(3);
			} else {
				init_to_station(dns_servers[attempt]);
			}

		} else {

			if(recvfrom (sockfd, message, DNS_MESSAGE_SIZE, 0, (struct sockaddr*) &to_station, (socklen_t*) &size_sock) < 0) {
				
        		attempt++;

        		if (attempt > dns_servers_no) {
					exit(3);
				} else {
					init_to_station(dns_servers[attempt]);
				}
    		} else {
				selected_DNS = attempt;
    			return ;
    		}
		}
	}
}

int read_qname_in_name(unsigned short offset) {
	
	int raw_size = 0;

	memset(&name_ans, 0, strlen(name_ans));

	int i = 0;
	int j = 0;

	while (1) {
		if (message[j + offset] == 0) {
			name_ans[i] = '.';
			name_ans[i] = '\0';
			break;
		}

		if (message[j + offset] >= 192) {

			unsigned char second_byte = message[j + offset + 1];
			unsigned short offset_2 = message[j + offset];

			offset_2 = offset_2 << 10;
			offset_2 = offset_2 >> 2;
			offset_2 = offset_2 + second_byte;

			char *copy = (char*) malloc(sizeof(char) * 256);

			strcpy(copy, name_ans);
			memset(&name_ans, 0, strlen(name_ans));
			name_ans[0]= '\0';

			read_qname_in_name(offset_2);

			int len = strlen(copy);

			for (int i = 0; i < strlen(name_ans); i++) {
				copy[len + i] = name_ans[i];
			}

			copy[len + strlen(name_ans)] = '\0';
			strcpy(name_ans, copy);

			memset(copy, 0, strlen(copy));
			free(copy);

			return raw_size + 1;
		}

		unsigned char seq = message[j + offset];
		j++;
		raw_size += seq;

		for (int k = 0; k < seq; k++) {

			name_ans[i] = message[j + offset];
			i++;
			j++;
		}

		name_ans[i] = '.';
		i++;
	}

	return raw_size;
}

unsigned char* read_entry(unsigned short *total_offset_copy, unsigned char *p, FILE* fd_dns_log, dns_rr_t dns_rr) {

	unsigned short total_offset = *total_offset_copy;

	unsigned char first_byte = *p;

	// Daca primii doi biti sunt 1, atunci urmeaza pointer
	if (first_byte >= 0xc0) {
			
		unsigned char second_byte = *(p + 1);
		unsigned short offset = first_byte;
			
		offset = offset << 10;
		offset = offset >> 2;
		offset = offset + second_byte;

		read_qname_in_name(offset);
		fprintf(fd_dns_log, "%s ", name_ans);

		p = p + 2; // sar peste cei doi bytes cititi
		total_offset += 2;

		first_byte  = *p;
		second_byte = *(p + 1);
		dns_rr.type  = first_byte << 8;
		dns_rr.type += second_byte;

		p = p + 2; // sar peste cei doi bytes cititi
		total_offset += 2;

		first_byte  = *p;
		second_byte = *(p + 1);
		dns_rr.class_z  = first_byte << 8;
		dns_rr.class_z += second_byte;

		p = p + 2 + 4; // sar peste cei doi bytes cititi + TTL
		total_offset += 6;

		first_byte  = *p;
		second_byte = *(p + 1);
		dns_rr.rdlength  = first_byte << 8;
		dns_rr.rdlength += second_byte;
			
		p = p + 2; // sar peste cei doi bytes cititi
		total_offset += 2;

		if (dns_rr.class_z == 0x0001) {
			fprintf(fd_dns_log, "IN ");
		} else {
			fprintf(fd_dns_log, "CLASS_z:%u ", dns_rr.class_z);
		}

		switch(dns_rr.type) {
			case A:

				fprintf(fd_dns_log, "A ");
				unsigned char ip_1 = *p;
				unsigned char ip_2 = *(p + 1);
				unsigned char ip_3 = *(p + 2);
				unsigned char ip_4 = *(p + 3);
				unsigned char ip_ans[20];
				sprintf(ip_ans, "%d.%d.%d.%d", ip_1, ip_2, ip_3, ip_4);

				fprintf(fd_dns_log, "%s\n", ip_ans);
				p = p + 4; // sar peste ip-ul citit
				total_offset += 4;

				break;

			case NS:
				fprintf(fd_dns_log, "NS ");

				read_qname_in_name(total_offset);
				fprintf(fd_dns_log, "%s\n", name_ans);

				p = p + dns_rr.rdlength;
				total_offset += dns_rr.rdlength;

				break;

			case CNAME:
				fprintf(fd_dns_log, "CNAME ");

				read_qname_in_name(total_offset);
				fprintf(fd_dns_log, "%s\n", name_ans);

				p = p + dns_rr.rdlength;
				total_offset += dns_rr.rdlength;

				break;

			case MX:
				fprintf(fd_dns_log, "MX ");

				first_byte  = *p;
				second_byte = *(p + 1);
				unsigned short pref  = first_byte << 8;
				pref                += second_byte;

				fprintf(fd_dns_log, "%d ", pref);

				// Sar peste cei 2 bytes cititi
				p += 2;
				total_offset += 2;

				read_qname_in_name(total_offset);

				p = p + dns_rr.rdlength - 2;
				total_offset += dns_rr.rdlength - 2;

				break;

			case SOA:

				fprintf(fd_dns_log, "SOA ");

				// Primary NS
				int size_1 = read_qname_in_name(total_offset);
				fprintf(fd_dns_log, "%s ", name_ans);

				p = p + size_1 + 1;
				total_offset += size_1 + 1;

				// Admin MB
				int size_2 = read_qname_in_name(total_offset);
				fprintf(fd_dns_log, "%s ", name_ans);

				p = p + size_2 + 1;
				total_offset += size_2 + 1;

				// 5 * Unsigned 32-bit integer.
				for (int b = 1; b <= 5; b++) {
					unsigned int val = *p;

					fprintf(fd_dns_log, "%u ", ntohs(val));
					p += 4;
					total_offset += 4;
				}

				if (size_2 + size_1 + 32 != dns_rr.rdlength) {
					p = p - size_1 - size_2 + dns_rr.rdlength;
					total_offset = total_offset - size_1 - size_2 + dns_rr.rdlength;
				}

				break;

			case TXT:
				fprintf(fd_dns_log, "TXT ");
	
				for (int i = 0; i < dns_rr.rdlength - 1; i++) {
					fprintf(fd_dns_log, "%c", *(p + 1 + i));
				}


				fprintf(fd_dns_log, "\n");

				p = p + dns_rr.rdlength;
				total_offset += dns_rr.rdlength;

				break;

			case PTR:
				fprintf(fd_dns_log, "PTR ");
				// TODO

				p = p + dns_rr.rdlength;
				total_offset += dns_rr.rdlength;

				break;

			case AAAA:
				p = p + 16; // doar sar peste aceasta zona de memorie si nu afisez nimic
				total_offset += 16;
				break;

			default:
				exit(5);
		}

	}

	*total_offset_copy = total_offset;

	return p;
}

void decompress_and_print_message() {

	FILE* fd_dns_log = fopen(FILE_NAME_DNS_LOG, "a");
    dns_header_t* dns_header_ptr = (dns_header_t *) message;
 
    int answers     = ntohs(dns_header_ptr->ancount);
    int auth_serves = ntohs(dns_header_ptr->nscount);
    int add_records = ntohs(dns_header_ptr->arcount);
    unsigned short total_offset = sizeof(dns_header_t) + strlen(qname) + 1 + sizeof(dns_question_t);
    dns_rr_t dns_rr;

	// Pointer catre inceputul zonei de raspuns
	unsigned char *p = &(*(message + sizeof(dns_header_t) + strlen(qname) + 1 + sizeof(dns_question_t)));

	if (answers > 0) {
		fprintf(fd_dns_log, "\n;; ANSWER SECTION:\n");
	}

    while (answers > 0) {
    	p = read_entry(&total_offset, p, fd_dns_log, dns_rr);
		answers--;
    }

    if (auth_serves > 0) {
		fprintf(fd_dns_log, "\n;; AUTHORITY SECTION:\n");
    }

    while (auth_serves > 0) {
    	p = read_entry(&total_offset, p, fd_dns_log, dns_rr);
		auth_serves--;
    }

    if (add_records > 0) {
		fprintf(fd_dns_log, "\n;; ADDITIONAL SECTION:\n");
    }

    while (add_records > 0) {
    	p = read_entry(&total_offset, p, fd_dns_log, dns_rr);
		add_records--;
    }

    fclose(fd_dns_log);
}

void log_message() {

	FILE* fd_message_log = fopen(FILE_NAME_MESSAGE_LOG, "a");

	int len = sizeof(dns_header_t) + sizeof(dns_question_t) + strlen(qname) + 1;

	for (int i = 0; i < len; i++) {
		fprintf(fd_message_log, "0x%.2x ", (char) message[i] & 0xff);
	}

	fprintf(fd_message_log, "\n");
}

void log_dns_header(char* recv_address, char* recv_type) {
	
	FILE* fd_dns_log = fopen(FILE_NAME_DNS_LOG, "a");

	fprintf(fd_dns_log, "; %s - %s %s\n", dns_servers[selected_DNS], recv_address, recv_type);
	fclose(fd_dns_log);
}

void log_dns_tailer() {
	FILE* fd_dns_log = fopen(FILE_NAME_DNS_LOG, "a");

	fprintf(fd_dns_log, "\n\n");
	fclose(fd_dns_log);
}

int main(int argc, char** argv) {

	// Verific numarul de argumente
	check_argc(argc);

	// Citesc lista de servere dns
	read_dns_servers();

	// Convertesc tipul: char -> int
	find_type(argv[2]);

	// Convertesc nume de adresa in qname
	set_qname(argv[1], strlen(argv[1]));

	// Intializez socketul UDP
	init_socket();

	// Creez cererea pentru DNS
	compress_query();
	
	// Scriu valorile HEX ce urmeaza a fi trimise catre DNS
	log_message();

	// Incerc sa realizez o conexiune (trimitere + raspuns)
	send_and_receive_query();

	// Loghez headerul pentru un raspuns
	log_dns_header(argv[1], argv[2]);

	// Analizez mesajul primit si il afisez corespunzator
	decompress_and_print_message();

	// Loghez tailerul pentru un raspuns
	log_dns_tailer();

	return 0;
}