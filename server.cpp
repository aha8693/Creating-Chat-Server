#include <pthread.h>
#include <iostream>
#include <sstream>
#include <memory>
#include <set>
#include <vector>
#include <cctype>
#include <cassert>
#include "message.h"
#include "connection.h"
#include "user.h"
#include "room.h"
#include "guard.h"
#include "server.h"

////////////////////////////////////////////////////////////////////////
// Server implementation data types
////////////////////////////////////////////////////////////////////////

// Struct to hold data for each client
struct ClientData {
  Connection* connection;
  Server* server; 
  ~ClientData() { delete connection; }
};

////////////////////////////////////////////////////////////////////////
// Client thread functions
////////////////////////////////////////////////////////////////////////


/**
 * Checks the validity of a room name.
 * A valid room name:
 * - is at least one character in length
 * - contains only letters (a-z or A-Z) or digits (0-9)
 * - is less than 255 characters
 *
 * @param roomName The room name to check for validity.
 * @return True if the room name is valid, false otherwise.
 */
bool is_valid_room_username(const std::string &roomName) {
  if (roomName.empty() || roomName.length()+1 > Message::MAX_LEN) {
    return false; // Room name must not be empty
  }

  // Check if the room name contains only letters (a-z or A-Z) or digits (0-9)
  for (char c : roomName) {
    if (!std::isalnum(c)) {
      return false; // Room name contains an invalid character
    }
  }

  return true; // Room name is valid
}


/**
 * Handles the communication with a sender client.
 *
 * @param clientConnection The Connection object for the sender client.
 * @param server The Server object managing the connections.
 * @param user The User object representing the sender.
 */
void chat_with_sender(Connection *clientConnection, Server *server, User *user) {

  while (true) {
    Message receivedMessage;
    if (!clientConnection->receive(receivedMessage)) {
      // Handle error and terminate the thread
      clientConnection->send(Message(TAG_ERR, "Error receiving message"));
      break;
    }

    // Process received message from sender
    std::string roomName = "";

    if (receivedMessage.data.length() + 1 > receivedMessage.MAX_LEN) {
      clientConnection->send(Message(TAG_ERR, "Message is too long"));
    }

    if (receivedMessage.tag == TAG_ERR) {
      std::cerr << receivedMessage.data << std::endl;
      return;
    }

    else if (receivedMessage.tag == TAG_JOIN) {
      roomName = receivedMessage.data;
      if (is_valid_room_username(roomName)) {
        if (user->room_number.empty()) { // Assuming user already joined a room, first remove the user from the current room
          Room *alreadyJoinedRoom = server->find_or_create_room(user->room_number);
          alreadyJoinedRoom->remove_member(user);
        }

        Room *joinedRoom = server->find_or_create_room(roomName);
        joinedRoom->add_member(user);
        user->room_number = roomName;
        clientConnection->send(Message(TAG_OK, "joined room"));
      } else {
        clientConnection->send(Message(TAG_ERR, "Invalid Room number"));
      }
    }

    else if (receivedMessage.tag == TAG_SENDALL) {
      if (!((user->room_number).empty())) { // Assuming the user is already joined a room
        Room * joinedRoom = server->find_or_create_room(user->room_number);
        std::string messageText = receivedMessage.data;
        joinedRoom->broadcast_message(user->username, messageText);

        clientConnection->send(Message(TAG_OK, "sent"));
      } else {
        clientConnection->send(Message(TAG_ERR, "Not joined any room"));
      }

    }
    
    else if (receivedMessage.tag == TAG_LEAVE) {
      if (!(user->room_number.empty())) { // Assuming the user is already joined a room
        roomName = user->room_number;
        Room *joinedRoom = server->find_or_create_room(roomName);
        joinedRoom->remove_member(user);
        user->room_number = "";
        clientConnection->send(Message(TAG_OK, "Left the room"));
      } else {
        clientConnection->send(Message(TAG_ERR, "Not in a room"));
      }
    }

    else if (receivedMessage.tag == TAG_QUIT) {
      clientConnection->send(Message(TAG_OK, "Bye"));
      return;
    }

    else {
      clientConnection->send(Message(TAG_ERR, "Invalid tag"));
    }
  }
}


/**
 * Handles the communication with a receiver client.
 *
 * @param clientConnection The Connection object for the receiver client.
 * @param server The Server object managing the connections.
 * @param user The User object representing the receiver.
 */
void chat_with_receiver(Connection *clientConnection, Server *server, User *user) {
  Message receivedMessage = Message();
  Room *joinedRoom = nullptr;

  if (!clientConnection->receive(receivedMessage)) {
    // Handle error and terminate the thread
    clientConnection->send(Message(TAG_ERR, "Error receiving message"));
    return;
  }

  // Joining a room
  if (receivedMessage.tag == TAG_JOIN) {
    std::string roomName = receivedMessage.data;
    if (is_valid_room_username(roomName)) {
      if (user->room_number.empty()) { // Assuming user already joined a room, first remove the user from the current room
        Room *alreadyJoinedRoom = server->find_or_create_room(user->room_number);
        alreadyJoinedRoom->remove_member(user);
      }

      joinedRoom = server->find_or_create_room(roomName);
      joinedRoom->add_member(user);
      user->room_number = roomName;
      clientConnection->send(Message(TAG_OK, "joined room"));
    } else {
      clientConnection->send(Message(TAG_ERR, "Invalid Room number"));
    }
  } else {
    clientConnection->send(Message(TAG_ERR, "Tag Error"));
  }
  
  while (true) {
    // Dequeue the next message from the user's message queue
    Message *broadcastedMsg = user->mqueue.dequeue();
    if (broadcastedMsg != nullptr) {
      clientConnection->send(*broadcastedMsg);
      delete(broadcastedMsg);
    }
  }
  joinedRoom->remove_member(user);
}

namespace {
/**
 * Worker function for a client thread.
 * Depending on the login type (sender or receiver), it calls the appropriate
 * chat function (chat_with_sender or chat_with_receiver).
 *
 * @param arg The ClientData object containing the client's data.
 * @return nullptr.
 */
void *worker(void *arg) {
  pthread_detach(pthread_self());

  ClientData *clientData = static_cast<ClientData *>(arg);
  Connection *clientConnection = clientData->connection;
  Server *server = clientData->server;

  // Error Catching during login
  Message loginMessage;
  if (!clientConnection->receive(loginMessage) || 
      (loginMessage.tag != TAG_SLOGIN && loginMessage.tag != TAG_RLOGIN)) {
    // Handle error and terminate the thread
    clientConnection->send(Message(TAG_ERR, "Login Message Receive Error"));
    return nullptr;
  }

  if (!is_valid_room_username(loginMessage.data)) {
    clientConnection->send(Message(TAG_ERR, "Invalid username"));
    return nullptr;
  }

  // Depending on the login type, call the appropriate chat function
  User *user = new User(loginMessage.data);
  if (loginMessage.tag == TAG_SLOGIN) {
    clientConnection->send(Message(TAG_OK, "Logged in as a sender: " + user->username));
    chat_with_sender(clientConnection, server, user);
  } else if (loginMessage.tag == TAG_RLOGIN) {
    clientConnection->send(Message(TAG_OK, "Logged in as a receiver: " + user->username));
    chat_with_receiver(clientConnection, server, user);
  } else {
    clientConnection->send(Message(TAG_ERR, "Invalid tag for login"));
    return nullptr;
  }


  return nullptr;

}
}

////////////////////////////////////////////////////////////////////////
// Server member function implementation
////////////////////////////////////////////////////////////////////////

/**
 * Constructor for the Server class.
 * Initializes the server with the specified port and initializes the mutex.
 *
 * @param port The port number to bind the server socket.
 */
Server::Server(int port)
  : m_port(port)
  , m_ssock(-1) {
  pthread_mutex_init(&m_lock, nullptr);
}

/**
 * Destructor for the Server class.
 * Destroys the mutex.
 */
Server::~Server() {
  pthread_mutex_destroy(&m_lock);
}

/**
 * Starts listening for client connections on the server socket.
 * Uses open_listenfd to create the server socket.
 *
 * @return True if successful, false otherwise.
 */
bool Server::listen() {
  m_ssock = open_listenfd(std::to_string(m_port).c_str());

  if (m_ssock < 0) {
    std::cerr << "Error: Failed to create server socket\n";
    return false;
  }

  return true;
}


/**
 * Handles client connection requests.
 * Accepts client connections and spawns a new thread for each client.
 */
void Server::handle_client_requests() {
  while (true) {
    int client_fd = Accept(m_ssock, nullptr, nullptr);
    if (client_fd < 0) {
      std::cerr << "Client connection accept error" << std::endl;
      return;
    }

    // Create a new Connection object for the client
    Connection *client_connection = new Connection(client_fd);

    ClientData *clientdata = new ClientData{client_connection, this};

    pthread_t tid;
    if (pthread_create(&tid, NULL, worker, clientdata) != 0) {
      std::cerr << "Pthread Creation Error" << std::endl;
      return;
    }
  }
}

/**
 * Finds or creates a Room object with the specified room name.
 * If the room already exists, returns a pointer to the existing Room.
 * If the room doesn't exist, creates a new Room and returns a pointer to it.
 *
 * @param room_name The name of the room.
 * @return A pointer to the Room object.
 */
Room *Server::find_or_create_room(const std::string &room_name) {
  Guard guard(m_lock);

  // Check if the room already exists
  auto it = m_rooms.find(room_name);
  if (it != m_rooms.end()) {
    return it->second;
  }

  // Create a new room if it doesn't exist
  Room *newRoomPtr = new Room(room_name);
  m_rooms[room_name] = std::move(newRoomPtr);

  return newRoomPtr;
}
