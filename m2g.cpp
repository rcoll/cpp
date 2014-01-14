// USAGE 
// m2g <memcache server address> <graphite server address> <memcache variable name> <graphite metric name>

#include <iostream>
#include <stdio.h>
#include <string.h>
#include <string>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <algorithm>
#include <sstream>
#include <ctime>

using namespace std;

class tcp_client {
	private:
		int sock;
		std::string address;
		int port;
		struct sockaddr_in server;
	 
	public:
		tcp_client();
		bool conn(string, int);
		bool send_data(string data);
		string receive(int);
};
 
tcp_client::tcp_client() {
	sock = -1;
	port = 0;
	address = "";
}
 
bool tcp_client::conn(string address, int port) {
	if(sock == -1) {
		sock = socket(AF_INET, SOCK_STREAM , 0);
		
		if(sock == -1) {
			perror("Could not create socket");
		}
	}	else { 
	}
	
	if(inet_addr(address.c_str()) == -1) {
		struct hostent *he;
		struct in_addr **addr_list;
		
		if((he = gethostbyname(address.c_str())) == NULL) {
			herror("gethostbyname");
			cout << "Failed to resolve hostname\n";
			return false;
		}
		
		addr_list = (struct in_addr **) he->h_addr_list;
		
		for(int i = 0; addr_list[i] != NULL; i++) {
			server.sin_addr = *addr_list[i];
			break;
		}
	} else {
		server.sin_addr.s_addr = inet_addr(address.c_str());
	}
	
	server.sin_family = AF_INET;
	server.sin_port = htons(port);
	
	if(connect(sock, (struct sockaddr *)&server , sizeof(server)) < 0) {
		perror("connect failed. Error");
		return 1;
	}
	
	return true;
}

bool tcp_client::send_data(string data) {
	if(send(sock , data.c_str() , strlen(data.c_str()), 0) < 0) {
		perror("Send failed");
		return false;
	}
	
	return true;
}

string tcp_client::receive(int size=512) {
	char buffer[size];
	string reply;
	
	if(recv(sock, buffer, sizeof(buffer) , 0) < 0) {
		puts("Recieve failed");
	}
	
	reply = buffer;
	return reply;
}

bool invalidChar(char c) {
	return !(c >= 0 && c < 128);
}

void stripUnicode(string & str) {
	str.erase(remove_if(str.begin(), str.end(), invalidChar), str.end());
}

int main(int argc, char *argv[]) {
	tcp_client cl_memc;
	tcp_client cl_grap;
	
	// Get our arguements
	string mc_host = argv[1];
	string gr_host = argv[2];
	string mc_var = argv[3];
	string gr_metric = argv[4];
	string mc_resp;
	
	// Standard port numbers for memcached and graphite
	int mc_port = 11211;
	int gr_port = 2003;
	
	// Get the requested variable from memcached
	cl_memc.conn(mc_host, mc_port);
	cl_memc.send_data("get " + mc_var + "\r\n");
	mc_resp = cl_memc.receive(1024);
	cl_memc.send_data("delete " + mc_var + "\r\n");
	cl_memc.send_data("quit\r\n");
	
	// Extract only the result we need from memcached response
	std::stringstream memcstream(mc_resp);
	std::string line1;
	std::getline(memcstream, line1);
	std::string metric_val;
	std::getline(memcstream, metric_val);
	
	// What time is it?
	time_t seconds;
	time(&seconds);
	std::stringstream ss;
	ss << seconds;
	std::string str_timestamp = ss.str();
	
	stripUnicode(gr_metric);
	stripUnicode(metric_val);
	
	// Create the string to send to graphite
	std::string graphite_data = gr_metric + " " + metric_val + " " + str_timestamp + "\r\n";
	
	cl_grap.conn(gr_host, gr_port);
	cl_grap.send_data(graphite_data);
	
	return 0;
}
