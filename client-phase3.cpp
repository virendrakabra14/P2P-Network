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
#include <netdb.h>
#include <unistd.h>
#include <mutex>
#include <algorithm>
#include <openssl/md5.h>
// #include <winsock.h>
// #include <experimental/filesystem>
#include <filesystem>

using namespace std;
int block = 0;
int total_neighbors;
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
    int port, public_id, pvt_id;
    string ip = lo_ip;
};
std::mutex mtx;
// int* pvt_ids;
int *file_found_node;
int tot_server_req = 0;
unsigned char **hashes;
//#include "./newclient.cpp"
void client(neighbor *neigh, char *payload, int payload_len, self *self)
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

    char reply[2];

    while (recverr == -1)
        recverr = recv(sockfd, reply, 2, 0);
    neigh->pvt_id = uint8_t(reply[0]) * 256 + uint8_t(reply[1]);
    // pvt_ids[neigh->public_id - 1] =
    char conf = 1;
    senderr = -1;
    int pack_len = payload_len;
    while (senderr == -1)
        senderr = send(sockfd, &pack_len, 4, MSG_DONTWAIT);
    senderr = -1;
    while (senderr == -1)
        senderr = send(sockfd, payload, payload_len, MSG_DONTWAIT);

    recverr = -1;
    char file_found[search_files.size()];
    while (recverr == -1)
        recverr = recv(sockfd, file_found, search_files.size(), 0);
    for (int i = 0; i < search_files.size(); i++)
    {
        if (file_found[i] == 1)
        {
            mtx.lock();
            if (file_found_node[i] > neigh->pvt_id)
                ;
            {
                file_found_node[i] = neigh->pvt_id;
            }
            mtx.unlock();
        }
    }
    mtx.lock();
    block++;
    mtx.unlock();
    while (block != total_neighbors)
        ; // block this thread till all other threads reach here

    vector<char> packet;
    vector<string> file_names;
    int num_files = 0;
    for (int i = 0; i < search_files.size(); i++)
    {
        if (file_found_node[i] == neigh->pvt_id)
        {
            file_names.push_back(search_files[i]);
            num_files++;
            packet.push_back(search_files[i].length());
            for (int j = 0; j < search_files[i].length(); j++)
                packet.push_back(search_files[i][j]);
        }
    }
    pack_len = packet.size();
    senderr = -1;
    while (senderr == -1)
        senderr = send(sockfd, &pack_len, 4, MSG_DONTWAIT);
    senderr = -1;
    while (senderr == -1)
        senderr = send(sockfd, &packet[0], pack_len, MSG_DONTWAIT);
    for (int i = 0; i < file_names.size(); i++)
    {
        int packlen;
        recverr = -1;
        while (recverr == -1)
            recverr = recv(sockfd, &packlen, 4, 0);

        char file[packlen];
        recverr = 0;
        while (recverr < packlen)
        {
            int ret = recv(sockfd, &file[recverr], packlen - recverr, 0);
            if (ret == -1)
                continue;
            else
                recverr += ret;
        }
        for (int j = 0; j < search_files.size(); j++)
        {
            if (search_files[j] == file_names[i])
            {
                unsigned char *hash = new unsigned char[16];
                MD5((unsigned char *)file, packlen, hash);
                hashes[j] = hash;
            }
        }
        ofstream *write_file;

        if (self->directory[self->directory.length() - 1] == '/')
            write_file = new ofstream(self->directory + "Downloaded/" + file_names[i]);
        else
            write_file = new ofstream(self->directory + "/Downloaded/" + file_names[i]);
        write_file->write(file, packlen);
        write_file->close();
        delete write_file;
    }
    close(sockfd);
}

//#include "./server.cpp"
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
void request_handler(int sockfd, self *self)
{
    int recverr = -1, senderr = -1;
    char c[] = {char(self->private_id / 256), char(self->private_id % 256)};
    while (senderr == -1)
        senderr = send(sockfd, c, 2, MSG_DONTWAIT | MSG_NOSIGNAL);

    int packlen;
    while (recverr == -1)
        recverr = recv(sockfd, &packlen, 4, 0);

    char pack[packlen];
    recverr = -1;
    while (recverr == -1)
        recverr = recv(sockfd, &pack, packlen, 0);

    vector<string> files = req_parser(pack, packlen);
    char found_file_num[files.size()];
    for (int i = 0; i < files.size(); i++)
        for (int j = 0; j < self->file_list.size(); j++)
        {
            if (files[i] == self->file_list[j])
            {
                found_file_num[i] = 1;
                break;
            }
            else
                found_file_num[i] = 0;
        }
    senderr = -1;
    while (senderr == -1)
        senderr = send(sockfd, &found_file_num[0], files.size(), MSG_DONTWAIT | MSG_NOSIGNAL);
    recverr = -1;
    while (recverr == -1)
        recverr = recv(sockfd, &packlen, 4, 0);

    char filepack[packlen];
    recverr = -1;
    while (recverr == -1)
        recverr = recv(sockfd, &filepack, packlen, 0);
    files = req_parser(filepack, packlen);
    for (int i = 0; i < files.size(); i++)
    {
        ifstream file_stream;
        file_stream.exceptions(ifstream::badbit);
        try
        {
            if (self->directory[self->directory.length() - 1] == '/')
                file_stream.open(self->directory + files[i], std::ios::binary);
            else
                file_stream.open(self->directory + "/" + files[i], std::ios::binary);
        }
        catch (ifstream::failure)
        {
            perror("Client : Cannot Read File");
            return;
        }
        file_stream.seekg(0, ios::end);
        int payload_len = file_stream.tellg();
        char payload[payload_len];
        file_stream.seekg(0, ios::beg);
        file_stream.read(payload, payload_len);

        senderr = -1;
        while (senderr == -1)
            senderr = send(sockfd, &payload_len, 4, MSG_DONTWAIT);
        senderr = 0;
        while (senderr < payload_len)
        {
            int res = send(sockfd, &payload[senderr], payload_len - senderr, MSG_DONTWAIT);
            if (res == -1)
                continue;
            else
                senderr += res;
        }
    }

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
        string text = "";
        vector<int> line;
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
        std::sort(search_files.begin(), search_files.end());
    }
    catch (ifstream::failure)
    {
        exit(1);
    }
    config_file.close();
}

vector<char> create_packets()
{
    vector<char> packet;
    for (int i = 0; i < search_files.size(); i++)
    {
        packet.push_back(search_files[i].length());
        for (int j = 0; j < search_files[i].length(); j++)
            packet.push_back(search_files[i][j]);
    }
    return packet;
}

void my_connect(vector<neighbor *> neighbs, self *self)
{
    hashes = new unsigned char *[search_files.size()];
    file_found_node = new int[search_files.size()];
    for (int i = 0; i < search_files.size(); i++)
    {
        file_found_node[i] = 0;
    }
    vector<char> packet = create_packets();
    vector<thread> th_vec;
    for (int i = 0; i < neighbs.size(); i++)
    {
        vector<char> c;
        string ip = self->ip;

        thread th(client, neighbs[i], &packet[0], packet.size(), self);
        th_vec.push_back(move(th));
    }
    for (int i = 0; i < th_vec.size(); i++)
    {
        th_vec[i].join();
    }
    for (int i = 0; i < neighbs.size(); i++)
    {
        cout << "Connected to " << neighbs[i]->public_id << " with unique-ID " << neighbs[i]->pvt_id << " on port " << neighbs[i]->port << endl;
    }
    for (int i = 0; i < search_files.size(); i++)
    {
        if (file_found_node[i] != 0)
        {
            cout << "Found " << search_files[i] << " at " << file_found_node[i] << " with MD5 ";
            for (int j = 0; j < 16; j++)
                printf("%02x", hashes[i][j]);
            cout << " at depth 1" << endl;
        }
        else
            cout << "Found " << search_files[i] << " at 0 with MD5 0 at depth 0" << endl;
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
    if (self->directory[self->directory.length() - 1] == '/')
        filesystem::create_directory(self->directory + "Downloaded");
    else
        filesystem::create_directory(self->directory + "/Downloaded");

    thread server(server_main_thread, self);
    my_connect(neighbors, self);
    server.join();
}

/*
g++ -std=c++17 client_1.cpp
*/