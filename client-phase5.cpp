#include <iostream>
#include <stdlib.h>
#include <string>
#include <cstring>
#include <vector>
#include <fstream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <thread>
#include <chrono>
#include <unistd.h>     //close()
#include <algorithm>    //sorting

// #include <winsock.h>
// #include <experimental/filesystem>
#include <filesystem>

#include <openssl/md5.h>

#include <sys/types.h>
// #include <sys/socket.h>
// #include <iostream>
// #include <stdlib.h>
// #include <thread>
// #include <vector>
#include <netdb.h>
// #include <cstring>
// #include <fstream>
#include <stdio.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include <unistd.h>

using namespace std;

#define SIZE 512            // used while sending files... (client.cpp, server.cpp)

int phase = 5;

int total_neighbors;

string lo_ip = "127.0.0.1";     // NOTE: as mentioned in the responses sheet, all IP addresses have been assumed lo only

vector<string> search_files;

// const int search_depth = 1; // depth of file search -- phase=2,3 : depth=1, phase=4,5 : depth=2

struct self
{
    int port, public_id, private_id;
    string ip = lo_ip;
    string directory;
    vector<string> file_list;
};

struct neighbor
{
    int port, public_id, pvt_id;
    string ip = lo_ip;
};

bool cmp_pvt_id(const neighbor* a, const neighbor* b)
{
    return a->pvt_id < b->pvt_id;
}

vector<neighbor *> neighbors;
vector<neighbor *> neighbors_depth1;
vector<neighbor *> neighbors_depth2;

int status[4];          // gives the status of completion of each part,
                        // the next one can't be done unless one is "complete"
                        // e.g. sending neighbors' pvt ids can't be done before establishing initial connection with that neighbor
                        // used for the same purpose in server.cpp (request_code == 3)

// #include "./client.cpp"
// #include "./server.cpp"


// --- client file starts ---

void client(neighbor *neigh, char *payload, int payload_len, char *reply)
{
    struct sockaddr_in serv_info;
    int sockfd = -1, connerr = -1, senderr = -1, recverr = -1;
    inet_pton(AF_INET, neigh->ip.c_str(), &serv_info.sin_addr);

    serv_info.sin_family = AF_INET;
    serv_info.sin_port = htons(neigh->port);

    while (sockfd == -1)
    {
        sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    }

    while (connerr == -1)
    {
        connerr = connect(sockfd, (struct sockaddr *)&serv_info, sizeof(serv_info));
    }
    // connerr = connect(sockfd, (struct sockaddr *)&serv_info, sizeof(serv_info));
    // if (connerr<0) {
    //     perror("Error connect() ");
    //     return;
    //     // exit(errno);
    // }
    while (senderr == -1)
        senderr = send(sockfd, payload, payload_len, MSG_DONTWAIT);         //request_code in packet itself

    int recved = 0;
    while (recved < 4) {
        recverr = recv(sockfd, reply+recved, 4-recved, 0);        // will receive private_id of this neighbor
        if (recverr==-1) continue;
        else recved+=recverr;
    }

    int more_data = 4*(uint8_t(reply[2])*256 + uint8_t(reply[3]));

    recved=0;
    while (recved < more_data) {
        recverr = recv(sockfd, &reply[4+recved], more_data-recved, 0);
        if (recverr==-1) continue;
        else recved+=recverr;
    }

    close(sockfd);
}

void send_search_req(neighbor *neigh, char* packets,int packet_len, char *reply)
{
    struct sockaddr_in serv_info;
    int sockfd = -1, connerr = -1, senderr = -1, recverr = -1;
    inet_pton(AF_INET, neigh->ip.c_str(), &serv_info.sin_addr);

    serv_info.sin_family = AF_INET;
    serv_info.sin_port = htons(neigh->port);

    while (sockfd == -1)
    {
        sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    }

    while (connerr == -1)
    {
        connerr = connect(sockfd, (struct sockaddr *)&serv_info, sizeof(serv_info));
    }
        for(int k = 0;k <packet_len;k++)
        senderr = -1;
        char req_type = 1;
        while (senderr == -1)
        {
            senderr = send(sockfd, &req_type, 1, 0);        //request_code = 1 for file search request
        }
        senderr = -1;
        uint32_t len = packet_len;
        while (senderr == -1)
        {
            senderr = send(sockfd, &len, 4, 0);     // ?? check: actually might hv sent <4 ??
        }
        // cout << "CLIENT: " << "search_req packet_len "<< packet_len<< endl;
        senderr = -1;
        while (senderr == -1)
        {
            senderr = send(sockfd, packets, packet_len, 0);
            // cout << "CLIENT: " <<"sent with code 1"<<endl;
        }
        // std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        recverr = -1;
        while(recverr == -1)
            recverr = recv(sockfd, reply, search_files.size(), 0);          //check if recved all... -- make the server send #bytes first..
        
        // cout << "CLIENT: " <<"search_req reply ";
        // for (int i=0; i<search_files.size(); i++) {
        //     cout <<reply[i]+0;            
        // }
        // cout <<"from port " << neigh->port << endl;
    
    close(sockfd);

}


vector<unsigned char> write_file (char* packet, int packet_len, int sockfd, string dir_download) {

    //cout << "CLIENT: " <<"enter write_file" << endl;       // yes, entered

    unsigned char result[MD5_DIGEST_LENGTH];

    int recverr;
    // FILE *fp;

    // with a prefix "new_"

    // char prefix[5] = "new_";    // last byte for null char

    // char *filename = new char[sizeof(prefix)+packet_len+1];
    // for (int i=0; i<sizeof(prefix)-1; i++) {
    //     filename[i]=prefix[i];
    // }

    // for (int i=sizeof(prefix)-1; i<sizeof(prefix)+packet_len; i++) {
    //     filename[i] = packet[i-sizeof(prefix)+1];            //made it go into "Downloaded"
    // }

    // "new_" removed
    char* filename = new char[packet_len+1];
    for (int i=0; i<packet_len; i++) {
        filename[i] = packet[i];            //made it go into "Downloaded"
    }
    filename[packet_len] = '\0';
    string filename_str = filename;
    
    char buffer[SIZE];

    int recved_bytes=0;          // bytes actually received

    // fp = fopen(filename,"w");          // overwrite if already present
    ofstream out_stream;
    out_stream.open(dir_download+filename_str);

    // can modify this as:
    // server sends 4 bytes (int: file size), and we run loop while(received<file_size)

    while (1) {
        recverr = recv(sockfd, buffer, SIZE, 0);
        if (recverr == -1) continue;        // try again
        else {recved_bytes+=recverr;}
        if (recverr == 0) break;       // server has closed the connection === file sent fully
        // const string s = buffer;
        // cout << "CLIENT: " << s.length() << endl;
        // out_stream << s;
        out_stream.write(buffer,min(recverr,SIZE));
        // if (!out_stream) cout << "CLIENT: " <<"ERROR!!!!" << endl;
        bzero(buffer, SIZE);
    }

    // not closing the file was causing MD5 hashes to NOT match!!
    out_stream.close();

    // cout << "CLIENT: " << "received " << recved_bytes << " bytes, written into " << (dir_download+filename_str) << endl;

    // get md5 hash: SO

    int fd = open((dir_download+filename_str).c_str(),O_RDONLY);

    struct stat statbuf;
    fstat(fd, &statbuf);
    unsigned long filesize = statbuf.st_size;

    char* file_buffer = static_cast<char*>(mmap(0, filesize, PROT_READ, MAP_SHARED, fd, 0));
    MD5((unsigned char*) file_buffer, filesize, result);
    munmap(file_buffer, filesize);

    // cout << "CLIENT: " << "md5 result in func ";
    // for(int i=0; i <MD5_DIGEST_LENGTH; i++) {
    //         printf("%02x",result[i]);
    // }
    // cout << endl;

    vector<unsigned char> return_res(MD5_DIGEST_LENGTH);
    for (int i=0; i<MD5_DIGEST_LENGTH; i++) {
        return_res[i] = result[i];
    }

    return return_res;

}


vector<unsigned char> send_download_req (neighbor *neigh, char* packet, int packet_len, string dir_download) {
    struct sockaddr_in serv_info;
    int sockfd = -1, connerr = -1, senderr = -1, recverr = -1;
    inet_pton(AF_INET, neigh->ip.c_str(), &serv_info.sin_addr);

    serv_info.sin_family = AF_INET;
    serv_info.sin_port = htons(neigh->port);

    while (sockfd == -1)
    {
        sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    }

    while (connerr == -1)
    {
        connerr = connect(sockfd, (struct sockaddr *)&serv_info, sizeof(serv_info));
    }

    char req_type = 2;

    int sent_bytes = 0;         // number of bytes actually sent
    while (sent_bytes < 1) {
        senderr = send(sockfd, &req_type+sent_bytes, 1-sent_bytes, 0);        //request_code = 2 for file download request
        if (senderr==-1) continue;
        else sent_bytes+=senderr;
    }

    sent_bytes = 0;
    uint32_t len = packet_len;
    while (sent_bytes < 4) {
        senderr = send(sockfd, &len+sent_bytes, 4-sent_bytes, 0);     // checked: actually might hv sent <4
        if (senderr==-1) continue;
        else sent_bytes+=senderr;
    }

    sent_bytes = 0;
    while (sent_bytes < packet_len) {
        senderr = send(sockfd, packet+sent_bytes, packet_len-sent_bytes, 0);
        if (senderr==-1) continue;
        else sent_bytes+=senderr;
    }
    // cout << "CLIENT: " <<"sent with code 2 to port " << neigh->port << " for " << packet << endl;


    // std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    // returns the MD5 hash of downloaded file
    vector<unsigned char> result = write_file(packet,packet_len,sockfd,dir_download);

    close(sockfd);

    return result;

}


void send_2_pvt_req (neighbor *neigh, char* reply) {

    struct sockaddr_in serv_info;
    int sockfd = -1, connerr = -1, senderr = -1, recverr = -1;
    inet_pton(AF_INET, neigh->ip.c_str(), &serv_info.sin_addr);

    serv_info.sin_family = AF_INET;
    serv_info.sin_port = htons(neigh->port);

    while (sockfd == -1)
    {
        sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    }

    while (connerr == -1)
    {
        connerr = connect(sockfd, (struct sockaddr *)&serv_info, sizeof(serv_info));
    }

    char req_type = 3;          //request_code = 3 for depth-2 pvt id request

    int sent_bytes = 0;         // number of bytes actually sent
    while (sent_bytes < 1) {
        senderr = send(sockfd, &req_type+sent_bytes, 1-sent_bytes, 0);
        if (senderr==-1) continue;
        else sent_bytes+=senderr;
    }
    // cout << "CLIENT: " <<"sent with code 3 to port " << neigh->port << endl;

    // std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    recverr = -1;
    while(recverr == -1)
        recverr = recv(sockfd, reply, 1000, 0);          //hv to check if recved all... (server should send #bytes first)

    close(sockfd);

}

// --- client file ends ---


// --- server file starts ---

vector<string> req_parser(char *req, int req_len)
{
    vector<string> files;
    for (int i = 0; i < req_len; i++)
    {
        int len = req[i];
        string name = "";
        while (len != 0)
        {
            i++;
            name += req[i];
            len--;
        }
        files.push_back(name);
    }
    return files;
}

void send_file(string filename, int sockfd) {


    char data[SIZE] = {0};

    ifstream in_file(filename, ios::binary);
    in_file.seekg(0, ios::end);
    int file_size = in_file.tellg();
    in_file.close();

    FILE* fp = fopen(filename.c_str(), "r");

    int sent_bytes = 0;
    int n;

    // cout << "SERVER: " << filename << " " << file_size << "bytes" << endl;

    while(sent_bytes<file_size) {
        // cout << "file sent: " << send(sockfd, data, sizeof(data), 0) << endl;
        fread(data, SIZE, 1, fp);
        n = send(sockfd, data, min(file_size-sent_bytes,SIZE), 0);    // BEEJ PAGE 50
        // min used above so that exact number of bytes are sent
                                                    // https://stackoverflow.com/questions/5594042/c-send-file-to-socket
                                                    // to ensure "sent completely"
        if (n==-1) { continue; }
        sent_bytes+=n;
        // cout << n << " " << sent_bytes <<endl;
        bzero(data, SIZE);
    }

    // cout << "SERVER: " << "sending " << sent_bytes << " bytes, " << filename << " complete" << endl;

    //
    // calculate MD5 hash at server end (while sending...)
    // just for debugging

    // unsigned char result[MD5_DIGEST_LENGTH];
    // int fd = open(filename.c_str(),O_RDONLY);

    // struct stat statbuf;
    // fstat(fd, &statbuf);
    // unsigned long filesize = statbuf.st_size;

    // char* file_buffer = static_cast<char*>(mmap(0, filesize, PROT_READ, MAP_SHARED, fd, 0));
    // MD5((unsigned char*) file_buffer, filesize, result);
    // munmap(file_buffer, filesize);

    // cout << "SERVER: " << "sending md5 ";
    // for(int i=0; i <MD5_DIGEST_LENGTH; i++) {
    //         printf("%02x",result[i]);
    // }
    // cout << endl;

}

void request_handler(int sockfd, self *self, vector<neighbor*> neighbs, vector<neighbor*> neighbs_depth1, string dir_path)
{
    char request_type;
    int recverr = -1, senderr = -1;
    while (recverr == -1)
        recverr = recv(sockfd, &request_type, 1, 0);
    
    // cout<<"SERVER: "<<"req_type "<<0+request_type<<endl;

    if (request_type == 0)
    {
        char c[4 + 4*neighbs.size()];
        c[0] = char(self->private_id / 256);
        c[1] = char(self->private_id % 256);

        // sending this neigbour stuff below, for depth=2

        c[2] = char(neighbs.size() / 256);
        c[3] = char(neighbs.size() % 256);

        for (int j=0; j<neighbs.size(); j++) {
            c[4+j*4] = char(neighbs[j]->public_id / 256);
            c[5+j*4] = char(neighbs[j]->public_id % 256);
            c[6+j*4] = char(neighbs[j]->port / 256);
            c[7+j*4] = char(neighbs[j]->port % 256);
        }

        int sent_bytes = 0;         // number of bytes actually sent
        int senderr;
        while (sent_bytes < sizeof(c)) {
            senderr = send(sockfd, c+sent_bytes, sizeof(c)-sent_bytes, MSG_DONTWAIT | MSG_NOSIGNAL);
            if (senderr==-1) continue;
            else sent_bytes+=senderr;
        }

    }

    else if (request_type == 1)
    {
        
        while(1) {
            if (status[0]==1) break;
        }

        recverr = -1;
        uint32_t pack_len = 0;

        while (recverr == -1)
            recverr = recv(sockfd, &pack_len, 4, 0);
        // cout<<"SERVER: " << "packet len "<<pack_len << endl;
        //pack_len = pack_len;

        char packet[pack_len];
        uint32_t recv_ret = 0;
        while (recv_ret < pack_len) {       // since recv may fail to read all data at once.
            recverr = recv(sockfd, &packet[recv_ret]+recv_ret, pack_len-recv_ret, 0);
            if (recverr==-1) continue;
            else { recv_ret += recverr; }
        }

        vector<string> file_names = req_parser(packet, pack_len);

        // for (int i = 0; i < file_names.size(); i++)
        // {
        //     cout<<"SERVER: file_names " << file_names[i] << ' ';
        // }
        // cout << endl;
        char reply[file_names.size()];
        for (int i=0; i<file_names.size(); i++) {
            reply[i]=0;
        }
        // cout << "reply ";
        for (int i = 0; i < file_names.size(); i++)
        {
            for (int j = 0; j < self->file_list.size(); j++)
            {
                if (file_names[i] == self->file_list[j])
                {
                    reply[i] = 1;           //search request reply for i-th file asked...
                    break;
                }
            }
            // cout << reply[i]+0 << " ";
        }
        // cout << endl;
        senderr = -1;
        while (senderr == -1)
            senderr = send(sockfd,reply, file_names.size(), MSG_DONTWAIT | MSG_NOSIGNAL);
        // cout<<"SERVER: " << "hello" << endl;
    }

    else if (request_type == 2) {       // download request

        recverr = -1;
        uint32_t pack_len = 0;

        while (recverr == -1)
            recverr = recv(sockfd, &pack_len, 4, 0);
        // cout<<"SERVER: " << "packet len "<<pack_len << endl;

        char packet[pack_len+1];      // contains a single file name
        packet[pack_len] = '\0';
        uint32_t recv_ret = 0;
        while (recv_ret < pack_len) {       // since recv may fail to read all data at once.
            recverr = recv(sockfd, &packet[recv_ret]+recv_ret, pack_len-recv_ret, 0);
            if (recverr==-1) continue;
            else { recv_ret += recverr; }
        }
        
        // cout << "SERVER: " << "received request_code=2 for " << packet << ' ' << sizeof(packet) << endl;

        for (int i=0; i<self->file_list.size(); i++) {
            if (packet == self->file_list[i]) {
                // cout << "SERVER: " << "sending " << self->file_list[i] << endl;
                send_file(dir_path + self->file_list[i],sockfd);       // actually put in the complete address
            }
        }

    }

    else if (request_type == 3) {               // request for pvt_id of my neighbors

        while(1) {
            if (status[0]==1) break;            // only proceed after establishing connections with own neighbors
        }

        char c[2 + 4*neighbs_depth1.size()];

        // sending this neigbour stuff below, for depth=2

        // number of depth-1 neighbors
        c[0] = char(neighbs_depth1.size() / 256);
        c[1] = char(neighbs_depth1.size() % 256);

        for (int j=0; j<neighbs_depth1.size(); j++) {
            c[2+j*4] = char(neighbs_depth1[j]->public_id / 256);
            c[3+j*4] = char(neighbs_depth1[j]->public_id % 256);
            c[4+j*4] = char(neighbs_depth1[j]->pvt_id / 256);
            c[5+j*4] = char(neighbs_depth1[j]->pvt_id % 256);
        }

        // cout << "SERVER: " << "sending for req_3 ";
        // for (int i=0; i<sizeof(c); i+=2) {
        //     cout << uint8_t(c[i]) * 256 + uint8_t(c[i+1]) << ' ';
        // }
        // cout<<endl;

        int sent_bytes = 0;         // number of bytes actually sent
        int senderr;
        while (sent_bytes < sizeof(c)) {
            senderr = send(sockfd, c+sent_bytes, sizeof(c)-sent_bytes, MSG_DONTWAIT | MSG_NOSIGNAL);
            if (senderr==-1) continue;
            else sent_bytes+=senderr;
        }
    
    }

    // Added more handlers for different request types.
    close(sockfd);
}

void server_main_thread(self *self, vector<neighbor*> neighbs, vector<neighbor*> neighbs_depth1, string dir_path)
{
    struct addrinfo hints, *res, *curr_addr;
    struct sockaddr_storage client_addr;
    int backlog = 10, sockfd, binderr, listenerr;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    getaddrinfo(self->ip.c_str(), to_string(self->port).c_str(), &hints, &res);
    for (curr_addr = res; curr_addr != nullptr; curr_addr = curr_addr->ai_next)
    {
        sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (sockfd == -1)
        {
            perror("Server : Socket Creation Error");
            continue;
        }

        // next 4 lines for the "Port already in use" error -- Beej's guide
        int yes=1;
        if (setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof yes) == -1) {
            perror("setsockopt");
            exit(1);
        }

        binderr = bind(sockfd, curr_addr->ai_addr, curr_addr->ai_addrlen);
        if (binderr == -1)
        {
            perror("Server: Port Bind Failed");
            close(sockfd);
            continue;
        }
        break;
    }
    if (curr_addr == nullptr)
    {
        cout<<"SERVER: " << "Server : Unable To Start The Server" << endl;
        exit(1);
    }
    listenerr = listen(sockfd, backlog);
    if (listenerr == -1)
    {
        perror("Server : Listen Error");
    }
    while (1)
    {
        socklen_t sin_size = sizeof client_addr;
        int connfd = accept(sockfd, (struct sockaddr *)&client_addr, &sin_size);
        if (connfd == -1)
        {
            perror("Server : Error In Accept");
            continue;
        }
        thread response_thread(request_handler, connfd, self, neighbs, neighbs_depth1, dir_path);
        response_thread.join(); // Maybe use detach for safety of huge file transfers?
    }
}

// --- server file ends ---




// --- main file starts ---


vector<int> parse_line(string line) // returns a vector, after splitting 'line' at ' ','\0','\n','\r'
{
    int prev_offset = 0;
    string token;
    vector<int> parsed;
    for (int i = 0; i < line.length(); i++)
    {
        if (line[i] == ' ' || line[i] == '\0' || line[i] == '\n' || line[i] == '\r' || i == line.length() - 1)
        {
            if (i != line.length() - 1)
            {
                token = line.substr(prev_offset, i - prev_offset);
                prev_offset = i + 1;
                parsed.push_back(stoi(token));
            }
            else
            {
                token = line.substr(prev_offset, i - prev_offset + 1);
                prev_offset = i + 1;
                parsed.push_back(stoi(token));
            }
        }
    }
    return parsed;
}

void read_file(string config_file_name, self *self, vector<neighbor *> *neighbs, vector<neighbor*> *neighbs_depth1)
{
    ifstream config_file;
    config_file.exceptions(ifstream::badbit);
    try
    {
        config_file.open(config_file_name);
    }
    catch (ifstream::failure)
    {
        exit(1);
    }
    try
    {
        string text;
        vector<int> line;
        getline(config_file, text);
        line = parse_line(text);
        self->public_id = line[0];  // line 1 : id of this machine
        self->port = line[1];       // port to listen on for incoming connections
        self->private_id = line[2]; // unique-id, private to self

        getline(config_file, text);
        total_neighbors = stoi(text);
        getline(config_file, text);
        if (total_neighbors > 0)
        {
            line = parse_line(text);
            for (int i = 0; i < total_neighbors; i++)
            {
                neighbor *n = new neighbor();
                n->public_id = line[2 * i]; // id of the neighbor
                n->port = line[2 * i + 1];  // port the client is listening on
                neighbs->push_back(n);
                neighbs_depth1->push_back(n);
            }
        }
        getline(config_file, text);
        int num_files = stoi(text);
        for (int i = 0; i < num_files; i++)
        {
            getline(config_file, text);
            if (text[text.length() - 1] == '\r' || text[text.length() - 1] == ' ' || text[text.length() - 1] == '\n' || text[text.length() - 1] == '\0')
                text = text.substr(0, text.length() - 1);
            search_files.push_back(text);
        }
    }
    catch (ifstream::failure)
    {
        exit(1);
    }
    config_file.close();
}

void my_connect(vector<neighbor *> neighbs, vector<neighbor*> neighbs_depth1, self *self)
{
    vector<vector<char> > c(neighbs.size());
    vector<char*> replies(neighbs.size());

    for (int i = 0; i < neighbs.size(); i++)
    {
        //cout<<i<<" i" << endl;
        //vector<char> c;
        c[i].push_back(0);          //code 0 (for private id request)
        string ip = self->ip;
        int j = 0;
        c[i].push_back(ip.length());

        for (; j < ip.length(); j++)
        {
            c[i].push_back(ip[j]);
        }

        c[i].push_back(self->port / 256);
        c[i].push_back(self->port % 256);

        //char *reply = new char[2];
        replies[i] = new char[1000];        // don't know how many neighbors this neighbor has!
                                            // so 1000 byte buffer...

        client(neighbs[i], &(c[0][0]), j + 4, replies[i]);

        // Connected to 3 with unique-ID 4526 on port 5000
        cout << "Connected to " << neighbs[i]->public_id << " with unique-ID " << uint8_t(replies[i][0]) * 256 + uint8_t(replies[i][1]) << " on port " << neighbs[i]->port << endl;

        neighbs[i]->pvt_id = uint8_t(replies[i][0]) * 256 + uint8_t(replies[i][1]);
        for (int k=0; k<neighbs_depth1.size(); k++) {
            if (neighbs_depth1[k]->public_id == neighbs[i]->public_id) {
                neighbors_depth1[k]->pvt_id = neighbs[i]->pvt_id;
            }
        }

        // now adding depth 2 neighbors (only ports and public_ids)
        
        int next_neighbs_count = uint8_t(replies[i][2]) * 256 + uint8_t(replies[i][3]);
        // cout << "next_neighbs_count " << next_neighbs_count << endl;

        for (int k=0; k<next_neighbs_count; k++) {

            // cout << uint8_t(replies[i][4+4*k]) * 256 + uint8_t(replies[i][5+4*k]) << ' ' << uint8_t(replies[i][6+4*k]) * 256 + uint8_t(replies[i][7+4*k]) << endl;

            int temp_public_id = uint8_t(replies[i][4+4*k]) * 256 + uint8_t(replies[i][5+4*k]);
            bool already = false;       // already present?
            if (temp_public_id == self->public_id) already=true;
            for (int k1=0; k1<neighbors.size(); k1++) {
                if (neighbors[k1]->public_id == temp_public_id) already=true;
            }

            if (!already && phase>3) {             // add into neighbors' list if not already present
                // cout << "added" << endl;
                neighbor* n1 = new neighbor;
                n1->ip = lo_ip;
                n1->public_id = temp_public_id;
                n1->port = uint8_t(replies[i][6+4*k]) * 256 + uint8_t(replies[i][7+4*k]);
                neighbors.push_back(n1);
                neighbors_depth2.push_back(n1);
            }

        }

        delete[] replies[i];        // free up 1000 bytes memory

    }

    // cout << "final #neighbors " << neighbors.size() << endl;
    // for (int i=0; i<neighbors.size(); i++) {
    //     cout << neighbors[i]->port << ' ';
    // }
    // cout << endl;

}

vector<char> create_packets(self *self)
{
    vector<char> packet;
    // packet.push_back(0);     //request code = 1 sent in client.cpp functions...
    for (int i = 0; i < search_files.size(); i++)
    {
        packet.push_back(search_files[i].length());
        for (int j = 0; j < search_files[i].length(); j++)
            packet.push_back(search_files[i][j]);
    }
    return packet;
}

// search request to depth-1 or depth-2 neighbors only
void search_req(vector<neighbor *> neighbs, self *self, vector<vector<int> >& found)
{
    // vector<vector<int> > found(neighbs.size(),vector<int>(search_files.size(),-1));  // matrix of size (#neighbs)*(#search_files)
    // vector<vector<char>> packets;
    vector<char> packet = create_packets(self);
    char search_res[search_files.size()];
    for(int i = 0;i<search_files.size();i++)
        search_res[i] = 0;
    for (int i = 0; i < neighbs.size(); i++)
    {
        char reply[search_files.size()];        //found or not found
        send_search_req(neighbs[i], &packet[0], packet.size(), reply);
        for (int j = 0; j < search_files.size(); j++)
        {
            // if(reply[j] == 1)
            // {
            //     if(search_res==0)
            //     search_res = neighbs
            // }
            if ((reply[j]+0) == 1)
            {
                // cout << "Found " << search_files[j] << " at " << neighbs[i]->pvt_id << " with MD5 0 at depth 1" << endl;
                found[i][j]=1;      // j-th file found with i-th neighbor
            }
            // else
            // {
            //     cout << "Found " << search_files[j] << " at 0 with MD5 0 at depth 0 (NOT FOUND)" << endl;
            // }
        }
    }
    // return found;

    // for (int i=0; i<neighbs.size(); i++) {
    //     for (int j=0; j<search_files.size(); j++) {
    //         cout << found[i][j];
    //     }
    //     cout << endl;
    // }
    // cout << endl;

}

// download request to depth-1 or depth-2 neighbors only
void download_req(vector<neighbor *> neighbs, self *self, vector<vector<int> >& found, int depth, string dir_download, vector<bool>& downloaded, vector<vector<unsigned char> >& md5hashes) {

    // vector<bool> downloaded(search_files.size(),0);

    for (int j=0; j<(found.size()>0 ? found[0].size() : 0); j++) {

        for (int i = 0; i < neighbs.size(); i++)        //**NOTE: first arrange neighbors by ascending order of unique-ID (for phase-2 too)
                                                        //bcoz given in problem stmt: if found at multiple places, report one
                                                        //with least unique ID
        {

            if (found[i][j]==1 && !downloaded[j]) {

                vector<char> packet;
                for (int k=0; k<search_files[j].size(); k++) {
                    packet.push_back(search_files[j][k]);
                }
                //char reply[SIZE];        //size 512B block of file
                md5hashes[j] = send_download_req(neighbs[i], &packet[0], packet.size(), dir_download);
                // cout << "Downloaded file with MD5 ";
                // cout << "Found " << search_files[j] << " at " << neighbs[i]->pvt_id << " with MD5 ";
                // for(int i=0; i <MD5_DIGEST_LENGTH; i++) {
                //         printf("%02x",res[i]);
                // }
                // cout<<" at depth " << depth << endl;
                downloaded[j]=1;

            }  

        }

        // if (!downloaded[j]) {
        //     cout << "Found " << search_files[j] << " at 0 with MD5 0 at depth 0" << endl;
        // }

    }

}

// request for private id's of depth-2 neighbors (from depth-1 neighbors)
void depth2_pvt_req(vector<neighbor*> neighbs, vector<neighbor*> neighbs_depth1, vector<neighbor*>& neighbs_depth2, self* self) {

    vector<char*> replies(neighbs_depth1.size());

    for (int i=0; i<neighbs_depth1.size(); i++) {
        
        replies[i] = new char[1000];
        send_2_pvt_req (neighbs_depth1[i], replies[i]);

        int next_neighbs_count = uint8_t(replies[i][0]) * 256 + uint8_t(replies[i][1]);
        // cout << "next_neighbs_count " << next_neighbs_count << endl;

        for (int k=0; k<next_neighbs_count; k++) {

            // cout << "in depth2_pvt_req " << uint8_t(replies[i][2+4*k]) * 256 + uint8_t(replies[i][3+4*k]) << ' ' << uint8_t(replies[i][4+4*k]) * 256 + uint8_t(replies[i][5+4*k]) << endl;

            int temp_public_id = uint8_t(replies[i][2+4*k]) * 256 + uint8_t(replies[i][3+4*k]);

            for (int k1=0; k1<neighbs_depth2.size(); k1++) {
                if (neighbs_depth2[k1]->public_id == temp_public_id) {
                    neighbs_depth2[k1]->pvt_id = uint8_t(replies[i][4+4*k]) * 256 + uint8_t(replies[i][5+4*k]);
                }
            }

        }

        // cout << "got depth_2 pvt ids ";
        // for (int j=0; j<neighbs_depth2.size(); j++) {
        //     cout << neighbs_depth2[j]->pvt_id << ' ';
        // }
        // cout<<endl;

        delete[] replies[i];
    }

}



int main(int argc, char *argv[])
{

    // WSADATA ws;
    // if (WSAStartup(MAKEWORD(2,2),&ws) < 0) {
    //     cout << "WSA failed to initialize" << endl;
    // }
    // else {
    //     cout << "WSA initialized" << endl;
    // }

    string config_file_name;
    string dir_path;
    if (argc > 2)
    {
        config_file_name = argv[1];
        dir_path = argv[2];
    }
    else
    {
        cout << "Usage: <executable> <config-file> <directory-path>" << endl;
        exit(1);
    }
    self *self = new struct ::self();
    self->directory = dir_path;
    read_file(config_file_name, self, &neighbors, &neighbors_depth1); // fill details from the config file

    // for (int i=0; i<search_files.size(); i++) {
    //     cout << search_files[i] <<' ';
    // }
    // cout << endl;

    // added
    sort(search_files.begin(),search_files.end());

    string dir_download = dir_path+"Downloaded/";
    // create "Downloaded" folder if it doesn't exist, delete if it exists
    if (filesystem::exists(dir_download)) {
        filesystem::remove_all(dir_download);
    }
    if (!filesystem::is_directory(dir_download) || !filesystem::exists(dir_download)) {
        filesystem::create_directory(dir_download);
    }

    // recursively iterate over directory, and sub-directories
    // get all file names
    for (const auto &entry : filesystem::recursive_directory_iterator(dir_path))
    {
        if (entry.is_directory())
            continue;
        string name = entry.path();
        for (int i = name.length() - 1; i >= 0; i--)
        {
            if (name[i] == '/')
            {
                name = name.substr(i + 1, name.length() - i); // extract just the file name
                                                              // from, e.g. ./files/client1/f1.txt
                break;
            }
        }
        cout << name << endl;
        self->file_list.push_back(name);
    }

    // cout << "file_list ";
    // for (int i=0; i<self->file_list.size(); i++) {
    //     cout << self->file_list[i] <<' ';
    // }
    // cout << endl;

    // cout << "#neigbours " << neighbors.size() << endl;

    // one corner case... no neighbors
    if (neighbors.size()==0) {
        for (int i=0; i<search_files.size(); i++) {
            cout << "Found " << search_files[i] << " at 0 with MD5 0 at depth 0" << endl;
        }
        exit(0);
    }

    thread server(server_main_thread, self, neighbors, neighbors_depth1, dir_path);

    my_connect(neighbors, neighbors_depth1, self);
    status[0] = 1;
    // std::this_thread::sleep_for(std::chrono::milliseconds(10));

    sort(neighbors_depth1.begin(),neighbors_depth1.end(),cmp_pvt_id);

    vector<vector<int> > found_dep1(neighbors_depth1.size(),vector<int>(search_files.size(),0));
    search_req(neighbors_depth1, self, found_dep1);
    status[1] = 1;

    vector<bool> downloaded(search_files.size(),0);

    vector<vector<unsigned char> > md5hashes(search_files.size(),vector<unsigned char>(MD5_DIGEST_LENGTH));

    download_req(neighbors_depth1,self,found_dep1,1,dir_download,downloaded,md5hashes);
    status[2] = 1;

    // for (int j=0; j<(found.size()>0 ? found[0].size() : 0); j++) {

    //     for (int i = 0; i < neighbors_depth1.size(); i++) {

    //         if (found[i][j]==1 && downloaded[j]) {

    //             cout << "Found " << search_files[j] << " at " << neighbors_depth1[i]->pvt_id << " with MD5 ";
    //             for(int k=0; k <MD5_DIGEST_LENGTH; k++) {
    //                     printf("%02x",md5hashes[j][k]);
    //             }
    //             cout<<" at depth 1" << endl;

    //         }

    //     }

    //     if (!downloaded[j]) {
    //         cout << "Found " << search_files[j] << " at 0 with MD5 0 at depth 0" << endl;
    //     }

    // }

    depth2_pvt_req(neighbors,neighbors_depth1,neighbors_depth2,self);
    status[3] = 1;

    sort(neighbors_depth2.begin(),neighbors_depth2.end(),cmp_pvt_id);
    vector<vector<int> > found_dep2(neighbors_depth2.size(),vector<int>(search_files.size(),0));
    
    search_req(neighbors_depth2, self, found_dep2);

    download_req(neighbors_depth2,self,found_dep2,2,dir_download,downloaded,md5hashes);

    vector<vector<int> > found;
    for (int i=0; i<found_dep1.size(); i++) {
        found.push_back(found_dep1[i]);
    }
    for (int i=0; i<found_dep2.size(); i++) {
        found.push_back(found_dep2[i]);
    }

    for (int j=0; j<(found.size()>0 ? found[0].size() : 0); j++) {      // index into search_files

        bool done = false;

        for (int i = 0; i < neighbors_depth1.size(); i++) {

            if (!done && found[i][j]==1 && downloaded[j]) {

                cout << "Found " << search_files[j] << " at " << neighbors_depth1[i]->pvt_id << " with MD5 ";
                for(int k=0; k <MD5_DIGEST_LENGTH; k++) {
                        printf("%02x",md5hashes[j][k]);
                }
                cout<<" at depth 1" << endl;

                done = true;

            }

        }

        if (!done) {

            for (int i = neighbors_depth1.size(); i < neighbors_depth1.size()+neighbors_depth2.size(); i++) {

                if (!done && found[i][j]==1 && downloaded[j]) {

                    cout << "Found " << search_files[j] << " at " << neighbors_depth2[i-neighbors_depth1.size()]->pvt_id << " with MD5 ";
                    for(int k=0; k <MD5_DIGEST_LENGTH; k++) {
                            printf("%02x",md5hashes[j][k]);
                    }
                    cout<<" at depth 2" << endl;

                    done = true;

                }

            }

        }

        if (!downloaded[j]) {
            cout << "Found " << search_files[j] << " at 0 with MD5 0 at depth 0" << endl;
        }

    }

    // for (int i=0; i<neighbors_depth1.size(); i++) {
    //     cout << neighbors_depth1[i]->public_id <<' ' << neighbors_depth1[i]->port << ' ' << neighbors_depth1[i]->pvt_id << endl;
    // }

    // for (int i=0; i<neighbors_depth2.size(); i++) {
    //     cout << neighbors_depth2[i]->public_id <<' ' << neighbors_depth2[i]->port << ' ' << neighbors_depth2[i]->pvt_id << endl;
    // }

    // added
    std::this_thread::sleep_for(std::chrono::milliseconds(10000));
    exit(0);

    server.join();

}

/*

sudo apt-get install openssl
sudo apt-get install libssl-dev

link libraries at the end: https://stackoverflow.com/questions/45135

g++ -g -std=c++17 -pthread client_2.cpp -lcrypto -lssl

./a.out config1.txt <path to client-1 files>
./a.out config2.txt <path to client-2 files>

Currently, clients 1,2,3 files pasted in this folder...
Topology: 1-3-2

---

Topology in given files:
2-3-1-4

Files to find:
1.
    bar.pdf
    file.cpp

2.
    f1.txt

3.
    random.py

4.
    foo.png

Files that each has:
1.
    f1.txt
    foo.png

2.
    bar.pdf
    random.py

3.
    another.txt

4.
    file.cpp

*/