#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netdb.h>
#include <cstring>
#include <cstdio>
#include <iostream>
#include <thread>
#include "socket_tools.h"


void processServerMesseges (int sfd) {

  while (true) {

    constexpr size_t buf_size = 1000;
    char buffer[buf_size] = {};

    sockaddr_in sin;
    socklen_t slen = sizeof(sockaddr_in);
    if (recvfrom(sfd, buffer, buf_size - 1, 0, (sockaddr*)&sin, &slen) > 0) {

      printf("Server: %s\n", buffer);
    }
  }
}


int main(int argc, const char **argv) {

  const char *port = "2025";

  addrinfo resAddrInfo;
  int sfd = create_dgram_socket("localhost", port, &resAddrInfo);

  if (sfd == -1) {

    printf("Cannot create a socket\n");
    return 1;
  }

  std::thread(processServerMesseges, sfd).detach();

  while (true) {

    std::string input;
    std::getline(std::cin, input);
    ssize_t res = sendto(sfd, input.c_str(), input.size(), 0, resAddrInfo.ai_addr, resAddrInfo.ai_addrlen);
    if (res == -1)
      std::cout << strerror(errno) << std::endl;
  }

  return 0;
}
