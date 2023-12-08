#include <sstream>
#include <cctype>
#include <cassert>
#include "csapp.h"
#include "message.h"
#include "connection.h"

Connection::Connection()
  : m_fd(-1)
  , m_last_result(SUCCESS) {
}

// Call rio_readinitb to initialize the rio_t object
Connection::Connection(int fd)
  : m_fd(fd)
  , m_last_result(SUCCESS) {
  Rio_readinitb(&m_fdbuf, fd);
}

// Call open_clientfd to connect to the server
// Call rio_readinitb to initialize the rio_t object
void Connection::connect(const std::string &hostname, int port) {
  m_fd = Open_clientfd(const_cast<char *>(hostname.c_str()), std::to_string(port).c_str());
  Rio_readinitb(&m_fdbuf, m_fd);
}

// Close the socket if it is open
Connection::~Connection() {
  if (is_open()) {
    close();
  }
}

// Return true if the connection is open
bool Connection::is_open() const {
  return m_fd >= 0;
}

// Close the connection if it is open
void Connection::close() {
  if (is_open()) {
    Close(m_fd);
    m_fd = -1;
  }
}


// Send a message
// return true if successful, false if not
// make sure that m_last_result is set appropriately
bool Connection::send(const Message &msg) {
  // Check if connection is open
  if (!is_open()) {
    m_last_result = EOF_OR_ERROR;
    return false;
  }

  // Check if the encoded message is not be more than MAX_LEN bytes.
  if (msg.tag.length() + msg.data.length() + 1 > msg.MAX_LEN) {
    m_last_result = INVALID_MSG; // Invalid message length
    return false;
  }

  const std::string send_msg = msg.tag + ":" + msg.data + "\n";
  const char* send_msg_chr = send_msg.c_str();

  if (ssize_t bytes_written = rio_writen(m_fd, send_msg_chr, send_msg.length()) < 1) {
    m_last_result = EOF_OR_ERROR;
    return false;
  }

  m_last_result = SUCCESS;
  return true;
}

bool Connection::receive(Message &msg) {
  // TODO: receive a message, storing its tag and data in msg
  // return true if successful, false if not
  // make sure that m_last_result is set appropriately

  // Check if connection is open
  if (!is_open()) {
    m_last_result = EOF_OR_ERROR;
    return false;
  }

  char server_msg_buf[msg.MAX_LEN +1];

  if (ssize_t bytes_read = Rio_readlineb(&m_fdbuf, server_msg_buf, msg.MAX_LEN) < 1) {
    m_last_result = EOF_OR_ERROR;
    return false;
  }

  std::string msg_tag, msg_data;

  std::istringstream iss(server_msg_buf);
  std::getline(iss, msg_tag, ':');
  std::getline(iss, msg_data);

  msg.tag = msg_tag;
  msg.data = msg_data;

  m_last_result = SUCCESS;
  return true;
}
