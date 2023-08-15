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
#include <unistd.h>
#include <sys/types.h>
#include <algorithm>
#include <netdb.h>
// #include <winsock.h>
// #include <experimental/filesystem>
#include <filesystem>

using namespace std;

int total_neighbors;
int tot_server_req = 0;
string lo_ip = "127.0.0.1";
vector<string> search_files;
const int search_depth = 1; // depth of file search
struct self
{
    int port, public_id, private_id;
    string ip = lo_ip;
    string directory;
    vector<string> file_list;
};
struct neighbor
{
    int port, public_id;
    string ip = lo_ip;
};

//#include "./client.cpp"
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
    while (senderr == -1)
        senderr = send(sockfd, payload, payload_len, MSG_DONTWAIT);

    while (recverr == -1)
        recverr = recv(sockfd, reply, 2, 0);
}

//#include "./server.cpp"
void request_handler(int sockfd, self *self)
{
    char request_type;
    recv(sockfd, &request_type, 1, 0);

    if (request_type == 0)
    {
        char c[] = {char(self->private_id / 256), char(self->private_id % 256)};
        send(sockfd, c, 2, MSG_DONTWAIT);
    }
    // Add more handlers for different request types.
    close(sockfd);
}
void server_main_thread(self *self)
{
    struct addrinfo hints, *res, *curr_addr;
    struct sockaddr_storage client_addr;
    int backlog = 10, sockfd, binderr, listenerr;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    // vector<thread> threads;
    int total_packets = 0;
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
        // cout << "Server : Unable To Start The Server"<< endl;
        exit(1);
    }
    listenerr = listen(sockfd, backlog);
    if (listenerr == -1)
    {
        perror("Server : Listen Error");
    }
    vector<thread> th;
    while (1)
    {

        socklen_t sin_size = sizeof client_addr;
        int connfd = accept(sockfd, (struct sockaddr *)&client_addr, &sin_size);
        tot_server_req++;
        if (connfd == -1)
        {
            perror("Server : Error In Accept");
            continue;
        }
        thread response_thread(request_handler, connfd, self);
        th.push_back(move(response_thread));
        if (tot_server_req == total_neighbors)
            break;
    }
    for (int i = 0; i < th.size(); i++)
    {
        th[i].join();
    }
    close(sockfd);
}
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
void read_file(string config_file_name, self *self, vector<neighbor *> *neighbors)
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
        text = "";
        while (text == "")
            getline(config_file, text);
        line = parse_line(text);
        self->public_id = line[0];  // line 1 : id of this machine
        self->port = line[1];       // port to listen on for incoming connections
        self->private_id = line[2]; // unique-id, private to self

        text = "";
        while (text == "")
            getline(config_file, text);
        total_neighbors = stoi(text);
        if (total_neighbors > 0)
        {
            text = "";
            while (text == "")
                getline(config_file, text);
            line = parse_line(text);
            for (int i = 0; i < total_neighbors; i++)
            {
                neighbor *n = new neighbor();
                n->public_id = line[2 * i]; // id of the neighbor
                n->port = line[2 * i + 1];  // port the client is listening on
                neighbors->push_back(n);
            }
        }
        text = "";
        while (text == "")
            getline(config_file, text);
        int num_files = stoi(text);
        for (int i = 0; i < num_files; i++)
        {
            text = "";
            while (text == "")
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

void my_connect(vector<neighbor *> neighbs, self *self)
{

    for (int i = 0; i < neighbs.size(); i++)
    {
        vector<char> c;
        c.push_back(0);
        char *reply = new char[2];
        client(neighbs[i], &c[0], 1, reply);
        // Connected to 3 with unique-ID 4526 on port 5000
        cout << "Connected to " << neighbs[i]->public_id << " with unique-ID " << uint8_t(reply[0]) * 256 + uint8_t(reply[1]) << " on port " << neighbs[i]->port << endl;
    }
}

int main(int argc, char *argv[])
{

    string config_file_name;
    string dir_path;
    if (argc > 2)
    {
        config_file_name = argv[1];
        dir_path = argv[2];
    }
    else
    {
        cout << "Usage: <executable> <config-file> <directory-path>\n";
        exit(1);
    }
    self *self = new struct ::self();
    self->directory = dir_path;
    vector<neighbor *> neighbors;
    read_file(config_file_name, self, &neighbors); // fill details from the config file

    // recursively iterate over directory, and sub-directories
    // get all file names
    vector<string> file_names;
    for (const auto &entry : filesystem::recursive_directory_iterator(dir_path))
    {
        int flag = 1;
        if (entry.is_directory())
            continue;
        string name = entry.path();
        for (int i = name.length() - 1; i >= 0; i--)
        {
            if (name[i] == '/')
            {
                if (name.length() > 11)
                    if (name.substr(i - 10, 10) == "Downloaded")
                        flag = 0;
                name = name.substr(i + 1, name.length() - i); // extract just the file name
                                                              // from, e.g. ./files/client1/f1.txt
                break;
            }
        }
        if (flag)
        {
            file_names.push_back(name);
            self->file_list.push_back(name);
        }
    }
    std::sort(file_names.begin(), file_names.end());
    for (int i = 0; i < file_names.size(); i++)
    {
        cout << file_names[i] << endl;
    }
    thread server(server_main_thread, self);
    my_connect(neighbors, self);

    server.join();
}

/*
g++ -std=c++17 -pthread client_1.cpp
*/