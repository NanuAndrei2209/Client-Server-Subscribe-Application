class Topic {
public:
	// topicul proriu-zis
	std::string topic;
	// store and forward
	int SF;
	Topic(std::string topic, int SF) {
		this->topic = topic;
		this->SF = SF;
	}
	Topic() {
		this->topic = "";
		this->SF = 0;
	}
	~Topic(){}
};

class Client {
public:
	// file descriptor-ul socket-ului prin care
	// se realizeaza conexiunea cu clientul respectiv
	int sockfd;

	//numele clientului
	std::string name;

	// online/offline
	bool connected;

	// lista cu topicuri la care este abonat
	std::vector<Topic> topics;

	// inbox pentru mesajele de la topicurile cu SF activ
	std::vector<std::string> inbox;
	
	Client(std::string name, int sockfd) {
		this->name = name;
		this->sockfd = sockfd;
		connected = true;
	}
	
	Client() {
		this->sockfd = 0;
		this->name = "";
		this->connected = false;
	}
	
	~Client(){}
};