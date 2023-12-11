#include "guard.h"
#include "message.h"
#include "message_queue.h"
#include "user.h"
#include "room.h"

// Constructor
Room::Room(const std::string &room_name)
  : room_name(room_name) {
  // Initialize the mutex
  pthread_mutex_init(&lock, NULL);
}

// Destructor
Room::~Room() {
  // Destroy the mutex
  pthread_mutex_destroy(&lock);
}

/**
 * Adds a user to the room.
 *
 * @param user The User object to add to the room.
 */
void Room::add_member(User *user) {
  Guard guard(lock);
  members.insert(user);
}

/**
 * Removes a user from the room.
 *
 * @param user The User object to remove from the room.
 */
void Room::remove_member(User *user) {
  Guard guard(lock);
  members.erase(user);
}

/**
 * Broadcasts a message to every user in the room.
 *
 * @param sender_username The username of the sender.
 * @param message_text The text of the message to broadcast.
 */
void Room::broadcast_message(const std::string &sender_username, const std::string &message_text) {
  Guard guard(lock);
  for (User *user : members) {
    user->mqueue.enqueue(new Message(TAG_DELIVERY, room_name + ":" + sender_username + ":" + message_text));
  }
}
