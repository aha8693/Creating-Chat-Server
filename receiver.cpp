#include <iostream>
#include <string>
#include <vector>
#include <stdexcept>
#include <sstream>
#include "csapp.h"
#include "message.h"
#include "connection.h"
#include "client_util.h"

int main(int argc, char **argv) {
  if (argc != 5) {
    std::cerr << "Usage: ./receiver [server_address] [port] [username] [room]\n";
    return 1;
  }

  std::string server_hostname = argv[1];
  int server_port = std::stoi(argv[2]);
  std::string username = argv[3];
  std::string room_name = argv[4];

  Connection conn;

  // Connect to server
  conn.connect(server_hostname, server_port);

  if (!conn.is_open()) {
    std::cerr << "Server Connection Failure" << std::endl;
    return 1;
  }

  /* Start of: Send rlogin message */ 
  Message rlogin_msg = Message(TAG_RLOGIN, username);
  if (!conn.send(rlogin_msg)) {
    std::cerr << "Message Send Failure: RLOGIN" << std::endl;
    return 2;
  }

  Message rlogin_msg_received = Message();
  
  if (!conn.receive(rlogin_msg_received)) {
    std::cerr << "Message Receive Failure: RLOGIN" << std::endl;
    return 2;
  }
  if (rlogin_msg_received.tag == TAG_ERR){
    std::cerr << rlogin_msg_received.data << std::endl; // output error message
    return 2;
  }
  /* End of: Send rlogin message */ 


  /* Start of: Send join message */
  if (!conn.send(Message(TAG_JOIN, room_name))) {
    std::cerr << "Message Send Failure: JOIN" << std::endl;
    return 2;
  }

  Message join_msg = Message(TAG_JOIN, room_name);
  
  if (!conn.receive(join_msg)) {
    std::cerr << "Message Receive Failure: JOIN" << std::endl;
    return 2;
  }
  if (join_msg.tag == TAG_ERR) {
    std::cerr << join_msg.data << std::endl; // output error message
    return 2;
  }
  /* End of: Send join message */



  // Loop waiting for messages from server (which should be tagged with TAG_DELIVERY)
  while (true) {
    Message received_msg = Message();
    /* Start of Error handling */
    if (!conn.receive(received_msg)) {
      std::cerr << "Message Receive Failure";
      return 2;
    }

    if (received_msg.tag == TAG_ERR) {
      std::cerr << received_msg.data; // output error message
      return 2;
    }
    /* End of Error handling */

    if (received_msg.tag == TAG_DELIVERY) {
      std::stringstream ss(received_msg.data);
      std::vector<std::string> tokens;
      std::string token;

      // Use ':' as the delimiter to split the string
      while (std::getline(ss, token, ':')) {
          tokens.push_back(token);
      }

      // Check if there are at least two tokens
      if (tokens.size() >= 2) {
          // Extract the last two tokens
          std::string last_two_tokens = tokens[tokens.size() - 2] + ": " + tokens[tokens.size() - 1];
          std::cout << last_two_tokens << std::endl;
      }

    }

  }


  return 0;
}
