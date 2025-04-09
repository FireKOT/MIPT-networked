#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <cstring>
#include <cstdio>
#include <iostream>
#include <string>
#include <list>
#include <algorithm>
#include <ctime>

#include "socket_tools.h"


bool isCommand (const std::string &msg, const std::string cmd) {

  //return msg.length() >= cmd.length() && !std::strncmp(msg.data(), cmd.data(), cmd.length());
  return msg == cmd;
}

bool isCommandWithParams (const std::string &msg, const std::string cmd) {

  return msg.length() > cmd.length() && !std::strncmp(msg.data(), cmd.data(), cmd.length());
}


class Server {

public:

  struct clientData {

    std::string name;

    sockaddr_in saddr;
    socklen_t slen;
  };

  enum class MathDuelState {

    NOBODY_CAME_TO_A_DUEL,
    WAITING_FOR_SECOND_DUELIST,
    ONGOING_DUEL,
  };

  explicit Server (int sfd): sfd_(sfd), clients_() {}

  void processMessage (std::string msg, sockaddr_in sin, socklen_t slen) {

    static std::string cmdStart = "/c ";
    static std::string cmdMathDuel = "/mathduel";
    static std::string cmdAns = "/ans ";

    clientData client{ "", sin, slen };

    if (isCommandWithParams(msg, cmdStart)) {
  
      std::string name(msg.data() + cmdStart.length());
      client.name = name;

      clients_.push_back(client);
      sendAll("Say hi to " + name + "!");
    }
    else if (isCommand(msg, cmdMathDuel)) {

      if (duelState_ == MathDuelState::NOBODY_CAME_TO_A_DUEL) {
        
        client.name = "First Duelist";
        firstDuelist_ = client;

        duelState_ = MathDuelState::WAITING_FOR_SECOND_DUELIST;

        sendMessage("Waiting for second duelist.", firstDuelist_);
      }
      else if (duelState_ == MathDuelState::WAITING_FOR_SECOND_DUELIST) {

        client.name = "Second Duelist";
        secondDuelist_ = client;

        processMathDuel(firstDuelist_, secondDuelist_);

        duelState_ = MathDuelState::ONGOING_DUEL;
      }
      else {

        sendMessage("Math Duel is currently running with other users.", client);
      }
    }
    else if (isCommandWithParams(msg, cmdAns)) {

      std::string answer(msg.data() + cmdAns.length());

      if (duelState_ == MathDuelState::ONGOING_DUEL) {

        if (firstDuelist_.saddr.sin_addr.s_addr == client.saddr.sin_addr.s_addr && firstDuelist_.saddr.sin_port == client.saddr.sin_port && firstDuelist_.saddr.sin_family == client.saddr.sin_family) {

          if (answer == ans_) {

            sendResults(firstDuelist_, secondDuelist_);
            duelState_ = MathDuelState::NOBODY_CAME_TO_A_DUEL;
          }
          else {

            sendMessage("Incorrect answer.", client);
          }
        }
        else if (secondDuelist_.saddr.sin_addr.s_addr == client.saddr.sin_addr.s_addr && secondDuelist_.saddr.sin_port == client.saddr.sin_port && secondDuelist_.saddr.sin_family == client.saddr.sin_family) {

          if (answer == ans_) {

            sendResults(secondDuelist_, firstDuelist_);
            duelState_ = MathDuelState::NOBODY_CAME_TO_A_DUEL;
          }
          else {

            sendMessage("Incorrect answer.", client);
          }
        }
        else {

          sendMessage("You are not participate in contest.", client);
        }
      }
      else {

        sendMessage("There is no active math duel.", client);
      }
    }
  }

  void processMathDuel (clientData firstDuelist, clientData secondDuelist) {

    int num1 = std::rand() % 100, num2 = std::rand() % 100;
    std::string task = "Solve: " + std::to_string(num1) + " + " + std::to_string(num2);
    ans_ = std::to_string(num1 + num2);

    sendMessage(task, firstDuelist);
    sendMessage(task, secondDuelist);
  }

  void sendResults (clientData winner, clientData looser) {

    sendMessage("You win!", winner);
    sendMessage("You loose!", looser);
  }

  void sendMessage (const std::string &msg, const clientData &client) {

      if (sendto(sfd_, msg.c_str(), msg.length(), 0, (sockaddr*) &client.saddr, client.slen) == -1) {

        std::cout << strerror(errno) << std::endl;
      }
  }

  void sendAll (const std::string &msg) {

    for (const auto &client: clients_) {

      sendMessage(msg, client);
    }
  }

private:

  int sfd_;

  std::list<clientData> clients_;

  MathDuelState duelState_ = MathDuelState::NOBODY_CAME_TO_A_DUEL;
  clientData firstDuelist_ = {};
  clientData secondDuelist_ = {};

  std::string ans_ = "";
};


int main(int argc, const char **argv) {

  std::srand(std::time(nullptr));

  const char *port = "2025";

  int sfd = create_dgram_socket(nullptr, port, nullptr);

  if (sfd == -1) {

    printf("cannot create socket\n");
    return 1;
  }
  printf("listening!\n");

  Server server(sfd);

  while (true) {

    fd_set readSet;
    FD_ZERO(&readSet);
    FD_SET(sfd, &readSet);

    timeval timeout = { 0, 100000 }; // 100 ms
    select(sfd + 1, &readSet, NULL, NULL, &timeout);


    if (FD_ISSET(sfd, &readSet)) {

      constexpr size_t buf_size = 1000;
      static char buffer[buf_size];
      memset(buffer, 0, buf_size);

      sockaddr_in sin;
      socklen_t slen = sizeof(sockaddr_in);
      ssize_t numBytes = recvfrom(sfd, buffer, buf_size - 1, 0, (sockaddr*)&sin, &slen);
      if (numBytes > 0) {

        printf("(%s:%d) %s\n", inet_ntoa(sin.sin_addr), sin.sin_port, buffer); // assume that buffer is a string

        server.processMessage(buffer, sin, slen);
      }
    }
  }

  return 0;
}
