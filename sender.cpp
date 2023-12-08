#include <iostream>
#include <string>
#include <sstream>
#include <stdexcept>
#include "csapp.h"
#include "message.h"
#include "connection.h"
#include "client_util.h"

int main(int argc, char **argv) {
  if (argc != 4) {
    std::cerr << "Usage: ./sender [server_address] [port] [username]\n";
    return 1;
  }

  std::string server_hostname;
  int server_port;
  std::string username;

  server_hostname = argv[1];
  server_port = std::stoi(argv[2]);
  username = argv[3];

  // Connect to server
  Connection conn;

  conn.connect(server_hostname, server_port);

  if (!conn.is_open()) {
    std::cerr << "Server Connection Failure" << std::endl;
    return 1;
  }

  /* Start of: Send rlogin message */ 
  Message slogin_msg = Message(TAG_SLOGIN, username);
  if (!conn.send(slogin_msg)) {
    std::cerr << "Message Send Failure: SLOGIN" << std::endl;
    return 2;
  }

  Message slogin_msg_receive = Message();
  
  if (!conn.receive(slogin_msg_receive)) {
    std::cerr << "Message Receive Failure: SLOGIN" << std::endl;
    return 2;
  }
  if (slogin_msg_receive.tag == TAG_ERR){
    std::cerr << slogin_msg_receive.data << std::endl; // output error message
    return 2;
  }
  /* End of: Send rlogin message */ 

  while (true) {
    std::string input;
    std::getline(std::cin, input);
    Message msg = Message();

    // Command Check
    if (input.substr(0, 6) == "/join ") {
      msg.tag = TAG_JOIN;
      msg.data = input.substr(6);
    } else if (input == "/leave") {
      msg.tag = TAG_LEAVE;
      msg.data = "";
    } else if (input == "/quit") {
      msg.tag = TAG_QUIT;
      msg.data = "";

      // Sender waiting for a reply from the server before exiting with exit code 0
      if (!conn.send(msg)) {
        std::cerr << "Message Send Failure: QUIT" << std::endl;
        return 2;
      }
      Message quit_msg = Message();
      if (!conn.receive(quit_msg)) {
        std::cerr << "Message Receive Failure: QUIT" << std::endl;
        return 2;
      }
      if (quit_msg.tag == TAG_ERR) {
        std::cerr << quit_msg.data << std::endl;
        return 2;
      }
      return 0;
    } else if (input[0] != '/') { // Input is a delievery 
      msg.tag = TAG_SENDALL;
      msg.data = input.substr(0, Message::MAX_LEN);
    } else {
      std::cerr << "Invalid commands: " << input << std::endl;
      continue;
    }

    // Sending a Message
    if (!conn.send(msg)) {
      std::cerr << "Message Send Failure: SENDALL" << std::endl;
      if (conn.get_last_result() == Connection::INVALID_MSG) { // message too long
        std::cerr << "Message is too long" << std::endl;
      }
      continue;
    }

    Message received_msg = Message();
    if (!conn.receive(received_msg)){
      std::cerr << "Message Receive Failure: SENDALL " << input << std::endl;
      continue;
    }
    if (received_msg.tag == TAG_ERR) {
      std::cerr << received_msg.data << std::endl;
      continue;
    }

  }

  conn.close();
  return 0;
}
