/*
 * Author : Group 2 (Sri ,Gayana, Gureaya)
 * Date - 11_05_2014
 * Description - This is the client of VMKit , getting
 * global counter information , function call frequency
 * and piloting them with gnuplot
 * */

#include <vector>
#include <cmath>
#include <cstdlib>
#include <string.h>
#include <cstring>
#include <unistd.h>
#include <stdio.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <strings.h>
#include <stdlib.h>
#include <string>
#include <time.h>
#include <boost/tuple/tuple.hpp>

#include "gnuplot-iostream.h"

using namespace std;

std::vector<std::string> &split(const std::string &s, char delim,
		std::vector<std::string> &elems) {
	std::stringstream ss(s);
	std::string item;
	while (std::getline(ss, item, delim)) {
		elems.push_back(item);
	}
	return elems;
}

std::vector<std::string> split(const std::string &s, char delim) {
	std::vector<std::string> elems;
	split(s, delim, elems);
	return elems;
}
void startClient() {

}

int main() {
	Gnuplot gp;
	Gnuplot gp1;

	std::vector<boost::tuple<int, int> > globalcounter;
	for (size_t i = 0; i < 20; i++) {
		int x = i;
		int y = 0;
		globalcounter.push_back(boost::make_tuple(x, y));
	}

	std::vector<boost::tuple<int, int> > functionfrequency;
	for (size_t i = 0; i < 20; i++) {
		int x = i;
		int y = 0;
		functionfrequency.push_back(boost::make_tuple(x, y));
	}

	gp
			<< "plot '-' with lines title 'globalcounter', '-' with lines title 'functionfrequency'\n";
	gp.send1d(globalcounter);
	gp.send1d(functionfrequency);

	int listenFd, portNo;
	bool loop = false;
	struct sockaddr_in svrAdd;
	struct hostent *server;

	//create client skt
	listenFd = socket(AF_INET, SOCK_STREAM, 0);

	if (listenFd < 0) {
		cerr << "Cannot open socket" << endl;
		exit(0);
	}

	server = gethostbyname("127.0.0.1");

	if (server == NULL) {
		cerr << "Host does not exist" << endl;
		exit(0);
	}

	bzero((char *) &svrAdd, sizeof(svrAdd));
	svrAdd.sin_family = AF_INET;

	bcopy((char *) server->h_addr, (char *) &svrAdd.sin_addr.s_addr, server -> h_length);

	svrAdd.sin_port = htons(9090);

	int checker = connect(listenFd, (struct sockaddr *) &svrAdd,
			sizeof(svrAdd));

	if (checker < 0) {
		cerr << "Cannot connect!" << endl;
		exit(0);
	}

	//send stuff to server
	string Buffer;
	for (;;) {
		char buffer[256];
		bzero(buffer, 256);
		int n = read(listenFd, buffer, 255);
		if (n < 0) {
			perror("ERROR reading from socket");
			exit(1);
		} else {
			string rcvbuffer(buffer);
			Buffer += rcvbuffer;
			int pos = Buffer.find_last_of(">");
			char processbuffer[100];
			memset(processbuffer, 0, sizeof(processbuffer));
			Buffer.copy(processbuffer, pos + 1);
			///cout << "This is the process buffer - " << processbuffer << endl;
			Buffer.erase(0, pos + 1);
			string processingstring(processbuffer);
			//if (!processingstring.empty()) {
				std::vector<string> processed = split(processingstring, '>');
				for (int i = 0; i < processed.size(); ++i) {
					char *pt = (char*) processed[i].c_str();
					if (*(pt + 1) == '1') {
						int val = atoi((char*) (pt + 3));
						int x = globalcounter[val].get<0>();
						int y = globalcounter[val].get<1>();
						int dy = atoi((char*) (pt + 5));
						globalcounter[val] = boost::make_tuple(x, dy);

					}
					if (*(pt + 1) == '2') {
						int val = atoi((char*) (pt + 3));
						int x = functionfrequency[val].get<0>();
						int y = functionfrequency[val].get<1>();
						int dy = atoi((char*) (pt + 5));
						functionfrequency[val] = boost::make_tuple(x, dy);
					}
					gp << "set xlabel 'Function index' \n";
					gp << "set title 'JIT function statistics' \n";
					gp
							<< "plot '-' with lines title 'globalcounter', '-' with lines title 'functionfrequency'\n";

					gp.send1d(globalcounter);
					gp.send1d(functionfrequency);

				}
			//}

		}
		printf("%s\n", buffer);
	}

}
