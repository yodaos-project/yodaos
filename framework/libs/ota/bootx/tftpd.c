#define _XOPEN_SOURCE 500

#include <stdio.h>
#ifdef WIN32
#include "winadapt.h"
#else
#include <unistd.h>
#include <strings.h>
#include <dirent.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#ifdef __APPLE__
#include <sys/select.h>
#endif

#include "tftpd.h"

static const char *blkstr = "blksize";

void diep(const char *s)
{
	perror(s);
	exit(1);
}

static int tftpd_thread(void *arg)
{
	struct tftpd_param *param = (struct tftpd_param*)arg;
	struct sockaddr_in listen_addr, client_addr;
	int listen_sock;
	fd_set input;
	struct timeval timeout;

	/* Initialize the timeout structure */
	timeout.tv_sec  = 1;
	timeout.tv_usec = 0;

	socklen_t slen = sizeof(client_addr);

	char rxbuf[TFTP_BUFLEN];

	if((listen_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
		diep("listen socket");

	memset((char *) &listen_addr, 0, sizeof(listen_addr));
	listen_addr.sin_family = AF_INET;
	listen_addr.sin_port = htons(param->port);
	listen_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(listen_sock, (struct sockaddr *) &listen_addr, sizeof(listen_addr)) == -1)
		diep("listen bind");

	while (!param->quit) {
		FD_ZERO(&input);
		FD_SET(listen_sock, &input);

		/* Do the select */
		if (select(listen_sock + 1, &input, NULL, NULL, &timeout) <= 0)
			continue;

		ssize_t recvd = recvfrom(listen_sock, rxbuf, TFTP_BUFLEN, 0,
				(struct sockaddr*)&client_addr, &slen);

		//		printf("Rx %d from %s:%d\n",
		//				(int) recvd,
		//				inet_ntoa(client_addr.sin_addr),
		//				ntohs(client_addr.sin_port));
		int handler_sock;
		struct sockaddr_in handler_addr;
		memset((char *) &handler_addr, 0, sizeof(handler_addr));
		handler_addr.sin_family = AF_INET;
		handler_addr.sin_port = htons(0);
		handler_addr.sin_addr.s_addr = INADDR_ANY;

		struct timeval tv;
		tv.tv_sec = 5;
		tv.tv_usec = 0;

		if((handler_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
			diep("handler socket");

		if(bind(handler_sock, (struct sockaddr *) &handler_addr, sizeof(handler_addr)) == -1)
			diep("handler bind");

		if(connect(handler_sock, (struct sockaddr *) &client_addr, sizeof(client_addr)) == -1)
			diep("connect");

		if (setsockopt(handler_sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv,  sizeof tv))
			diep("setsockopt");

		tftp_packet_t * request = parse_buffer(rxbuf, recvd);

		char * mode = request->body.rwrq.mode;
		if(strcasecmp(mode, "netascii") != 0 && strcasecmp(mode, "octet") != 0){
			printf("%s==netascii?%d\n%s==octet?%d\n", mode, strcasecmp(mode, "netascii"), mode, strcasecmp(mode, "octet"));
			send_error(E_OP, "invalid mode", handler_sock);
			exit(EXIT_FAILURE);
		}

		//print_packet(request);
		switch(request->opcode){
			case OP_WRQ:
				//puts("Receiving File");
				//receive_file(request, handler_sock);
				break;
			case OP_RRQ:
				char mess[20];
				int len;
				len = sprintf(mess, "%s%c%zd", blkstr, 0, request->body.rwrq.blksize);
				send_oack(mess, len, handler_sock);
				//puts("Sending File");
				send_file(request, handler_sock);
				break;
			default:
				//send_error(4, "Invalid Opcode", handler_sock);
				exit(EXIT_FAILURE);
		}
	}
	closesocket(listen_sock);
	return EXIT_SUCCESS;
}


void tftpd_start(struct tftpd_param *arg)
{
	arg->quit = 0;

	arg->thread = _CreateThread(tftpd_thread, arg);
}

void tftpd_stop(struct tftpd_param *arg)
{
	arg->quit = 1;
	_WaitThreadJoin(arg->thread);
}

void send_file(tftp_packet_t * request, int sock)
{
	//RRQ
	char * fname = request->body.rwrq.filename;

	FILE *rfile = fopen(fname, "rb");
	if(rfile == NULL){
		switch(errno){
			case EACCES:
				send_error(E_ACCESS, strerror(errno), sock);
				break;
			case ENOENT:
				send_error(E_NOFILE, strerror(errno), sock);
				break;
			default:
				send_error(100 + errno, strerror(errno), sock);
				break;
		}

		printf("No Found file: %s\n", fname);
		return;
	}

	size_t bsize = request->body.rwrq.blksize;

	int block = 1;
	size_t actual;
	char *read_buf = (char*)malloc(bsize);
	size_t extra = 0;
	char * ebuf = NULL;
	do {
		tftp_packet_t data_p;
		if(extra){

			char * new_data = (char*)malloc(bsize-extra);
			actual = fread(new_data, sizeof(char), bsize - extra, rfile);

			atona(&new_data, &actual);

			read_buf = (char*)realloc(read_buf, actual + extra);

			memcpy(read_buf, ebuf, extra);
			memcpy(read_buf + extra, new_data, actual);

			actual = extra + actual;

			free(new_data);
			free(ebuf);
			extra = 0;
		} else {
			actual = fread(read_buf, sizeof(char), bsize, rfile);

			if(strcasecmp(request->body.rwrq.mode, "netascii") == 0){
				atona(&read_buf, &actual);
			}
		}
		if(actual > bsize){
			extra = actual - bsize;
			actual = bsize;
			ebuf = (char*)malloc(extra);
			memset(ebuf, 0, extra);
			memcpy(ebuf, read_buf+bsize, extra);
		}

		data_p.opcode = OP_DATA;
		data_p.body.data.block_num = block;
		data_p.body.data.length = actual;
		data_p.body.data.data = read_buf;
		char * sbuf;
		size_t sbuf_size = prepare_packet(&data_p, &sbuf);

		char bsent = 0;
		int retries = 0;
		while(bsent == 0){
			send(sock, sbuf, sbuf_size, 0);


			char ack_buf[10];

			size_t recvd = recv(sock, ack_buf, 10, 0);

			if(recvd == (size_t)(-1) && errno == EAGAIN){
				if(retries >= 8){
					puts("timeout exit");
					exit(EXIT_FAILURE);
				}
				printf("retrying %d more times.\n", 8 -retries);
				retries++;
				continue;
			}

			tftp_packet_t *ack = parse_buffer(ack_buf, recvd);
			if(ack->opcode == OP_ACK && ack->body.ack.block_num == block){
				bsent = 1;
			}
			packet_free(ack);
		}
		free(sbuf);

		block++;
	} while (actual == bsize);
	free(read_buf);
	fclose(rfile);
}

void receive_file(tftp_packet_t * request, int sock)
{
#if 0	//will add later if necessary
	//WRQ
	char *fname =request->body.rwrq.filename;
	size_t bsize = request->body.rwrq.blksize;

	//check if file exists
	if(access(fname, F_OK) == 0){
		send_error(6, "File alread exists.", sock);
		puts("Exiting: File Exists");
		exit(EXIT_FAILURE);
	}
	FILE *dstfile = fopen(fname, "wb+");

	if(dstfile == NULL){
		switch(errno){
			case EACCES:
				send_error(E_ACCESS, strerror(errno), sock);
				puts("Exiting: Access");
				exit(EXIT_FAILURE);
			default:
				send_error(100 + errno, strerror(errno), sock);
				diep("recv fopen");
		}
	}



	int block = 1;

	char *rxbuf = (char*)malloc(bsize + 4);
	size_t recvd = 0;
	tftp_packet_t *packet = 0;
	int retries = 0;
	send_ack(0, sock); //start sendin'
	do {
		recvd = recv(sock, rxbuf, bsize+4, 0);
		if(recvd == (size_t)(-1) && errno == EAGAIN){
			if(retries >= 8){
				puts("timeout exit");
				exit(EXIT_FAILURE);
			}
			printf("retrying %d more times.\n", 8 -retries);
			retries++;
			continue;
		}

		packet = parse_buffer(rxbuf, recvd);
		if(packet->opcode != OP_DATA)
			send_error(E_OP, "Waiting for data", sock);
		if(packet->body.data.block_num == block){
			send_ack(block, sock);
			char * data = packet->body.data.data;
			size_t length = packet->body.data.length;
			if(strcasecmp(request->body.rwrq.mode, "netascii") == 0){
				natoa(&data, &length);
			}
			fwrite(data, 1, length, dstfile);
		}
		block++;
	} while (packet->body.data.length == bsize);
#endif
}

void send_ack(int block, int sock)
{
	tftp_packet_t *response = (tftp_packet_t*)malloc(sizeof(tftp_packet_t));
	response->opcode = OP_ACK;
	response->body.ack.block_num = block;
	char *sbuf;
	size_t blen =  prepare_packet(response, &sbuf);
	if( send(sock, sbuf, blen, 0)==-1)
		diep("ack sendto()");

	free(sbuf);
	packet_free(response);
}

void send_error(short code, const char *message, int sock)
{
	char *sbuf;

	tftp_packet_t *response = (tftp_packet_t*)malloc(sizeof(tftp_packet_t));
	response->opcode = OP_ERROR;
	response->body.error.error_code = code;
	response->body.error.errmsg = (char*)message;
	size_t len = prepare_packet(response, &sbuf);
	if(send(sock, sbuf, len, 0) == -1)
		diep("error sendto()");
	free(sbuf);
	packet_free(response);
}

void send_oack(char *message, int count, int sock)
{
	char *sbuf;

	tftp_packet_t *response = (tftp_packet_t*)malloc(sizeof(tftp_packet_t));
	response->opcode = OP_OACK;
	response->body.oack.data = message;
	response->body.oack.avail_len = count;
	size_t len = prepare_packet(response, &sbuf);
	int retries = 0;
	while(1){
		if(send(sock, sbuf, len, 0) == -1)
			diep("error sendto()");

		char ack_buf[10];
		size_t recvd = recv(sock, ack_buf, 10, 0);

		if(recvd == (size_t)(-1) && errno == EAGAIN){
			if(retries >= 8){
				puts("timeout exit");
				exit(EXIT_FAILURE);
			}
			printf("retrying %d more times.\n", 8 -retries);
			retries++;
			continue;
		}
		break;
	}
	free(sbuf);
	packet_free(response);
}

/*
 * This makes the assumption that if a block of data already has an \r it is
 * either binary data and should have been sent as such,
 * or it is already in netascii.
 */
void atona(char **rbuf, size_t *buflen)
{
	char * buf = *rbuf;
	char * temp = (char*)malloc(*buflen * 2);
	int result_index = 0, buf_index = 0;
	while(buf_index < (int)(*buflen)){
		if(buf[buf_index] == '\n'){
			temp[result_index++] = '\r';
			temp[result_index++] = buf[buf_index++];
		} else {
			temp[result_index++] = buf[buf_index++];
		}
	}
	*rbuf = temp;
	*buflen = result_index;
	free(buf);
}

void natoa(char ** rbuf, size_t *buflen)
{
	char *buf = *rbuf;
	char * temp = (char*)malloc(*buflen);
	memset(temp, 0, *buflen);
	int res_i = 0, buf_i = 0;
	while (buf_i < (int)(*buflen)){
		if(buf[buf_i] == '\r' && buf[buf_i+1] == '\n')
			buf_i++;
		temp[res_i++] = buf[buf_i++];
	}
	*rbuf = temp;
	free(buf);
	*buflen = res_i;
}

tftp_packet_t *parse_buffer(char * buffer, size_t length)
{
	tftp_packet_t * packet = (tftp_packet_t*)malloc(sizeof(tftp_packet_t));

	packet->opcode = ntohs(*((uint16_t*) buffer));

	switch(packet->opcode){
		case OP_RRQ:
		case OP_WRQ:
			{
				packet->body.rwrq.blksize = 512;
				char * fname = buffer + 2;
				char * mode = fname + strlen(fname) + 1;
				char * str = mode + strlen(mode) + 1;
				packet->body.rwrq.filename = (char*)malloc(sizeof(char) * strlen(fname)+1);
				strcpy( packet->body.rwrq.filename, fname);
				packet->body.rwrq.mode = (char*)malloc(sizeof(char) * strlen(fname)+1);
				strcpy(packet->body.rwrq.mode, mode);
				while (length > (unsigned int)(str - buffer)) {
					if (strcasecmp(str, blkstr) == 0) {
						size_t blksize;
						blksize = atoi((char*)&(str[strlen(blkstr) + 1]));
						packet->body.rwrq.blksize = blksize;
						break;
					}
					str += strlen(str) + 1;
				}
				return packet;
			}
		case OP_DATA:
			{
				packet->body.data.block_num = ntohs(* ((uint16_t*) (buffer + 2)));
				packet->body.data.length = length - 4;
				packet->body.data.data = (char*)malloc(sizeof(char) * packet->body.data.length);
				memcpy(packet->body.data.data, (buffer + 4), packet->body.data.length);
				return packet;
			}
		case OP_ACK:
			{
				packet->body.ack.block_num = ntohs(* ((uint16_t*) (buffer + 2)));
				return packet;
			}
		case OP_ERROR:
			{

			}
		case OP_OACK:
			{

			}
		default:
			packet->opcode = -1;
			return packet;
	}
}

size_t prepare_packet(tftp_packet_t *packet, char **rbuf)
{
	char *buf = 0;
	size_t bufsize = 0;
	switch(packet->opcode){
		case OP_RRQ:
		case OP_WRQ:
			{
				size_t f_len = strlen(packet->body.rwrq.filename);
				size_t m_len = strlen(packet->body.rwrq.mode);
				char * buf = (char*)malloc(bufsize = 4 + f_len + m_len);
				*(uint16_t*)(buf +0) = htons(packet->opcode);
				strcpy(buf+2, packet->body.rwrq.filename);
				strcpy(buf+2 + f_len, packet->body.rwrq.mode);
			}
			break;
		case OP_ACK:
			{
				buf = (char*)malloc(bufsize = sizeof(char)*4);
				*(uint16_t*)(buf +0) = htons(packet->opcode);
				*(uint16_t*)(buf +2) = htons(packet->body.ack.block_num);
			}
			break;
		case OP_DATA:
			{
				buf = (char*)malloc(bufsize = 4 + packet->body.data.length);
				*(uint16_t*)(buf +0) = htons(packet->opcode);
				*(uint16_t*)(buf +2) = htons(packet->body.ack.block_num);
				memcpy(buf+4, packet->body.data.data, packet->body.data.length);
			}
			break;
		case OP_ERROR:
			{
				buf = (char*)malloc(bufsize = 4 + strlen(packet->body.error.errmsg) + 1);
				*(uint16_t*)(buf +0) = htons(packet->opcode);
				*(uint16_t*)(buf +2) = htons(packet->body.error.error_code);
				strcpy(buf+4, packet->body.error.errmsg);
			}
			break;
		case OP_OACK:
			{
				buf = (char*)malloc(bufsize = 2 + packet->body.oack.avail_len);
				*(uint16_t*)(buf +0) = htons(packet->opcode);
				memcpy(buf+2, packet->body.oack.data, packet->body.oack.avail_len);
			}
			break;
	}
	*rbuf = buf;
	return bufsize;
}

void packet_free(tftp_packet_t *packet)
{
	switch(packet->opcode){
		case OP_RRQ:
		case OP_WRQ:
			free(packet->body.rwrq.filename);
			free(packet->body.rwrq.mode);
			break;
		case OP_DATA:
			free(packet->body.data.data);
			break;
		case OP_ERROR:
			// free(packet->body.error.errmsg);
			break;
		case OP_OACK:
			break;
	}
	free(packet);
}

void print_packet(tftp_packet_t * packet)
{
	switch(packet->opcode){
		case OP_DATA:
			printf("DATA - Opcode %d\tBlock: %d\nData Length: %d\nData:\n%.*s\n",
					packet->opcode,
					packet->body.data.block_num,
					(int)packet->body.data.length,
					(int)packet->body.data.length,
					packet->body.data.data
			      );
			break;
		case OP_WRQ:
			printf("WRQ :\tOpcode: %d\tFile: %s\tMode:%s\n",
					packet->opcode,
					packet->body.rwrq.filename, packet->body.rwrq.mode);
			break;
		case OP_RRQ:
			printf("RRQ :\tOpcode: %d\tFile: %s\tMode:%s\n",
					packet->opcode,
					packet->body.rwrq.filename, packet->body.rwrq.mode);
			break;
		case OP_ACK:
			printf("ACK :\tOpcode: %d\t Block: %d",
					packet->opcode,
					packet->body.ack.block_num);
			break;
		case OP_ERROR:
			printf("ERROR :\tOpcode: %d\nError Code: %d\tMsg: %s",
					packet->opcode,
					packet->body.error.error_code,
					packet->body.error.errmsg);
			break;
	}
}

