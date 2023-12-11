#include <cassert>
#include <ctime>
#include "message_queue.h"
#include "guard.h"

/**
 * Constructor for the MessageQueue class.
 * Initializes the mutex and semaphore used for synchronization.
 */
MessageQueue::MessageQueue() {
  pthread_mutex_init(&m_lock, NULL);
  sem_init(&m_avail, 0, 0);
}

/**
 * Destructor for the MessageQueue class.
 * Destroys the mutex and semaphore used for synchronization.
 */
MessageQueue::~MessageQueue() {
  pthread_mutex_destroy(&m_lock);
  sem_destroy(&m_avail);
}

/**
 * Enqueues a Message into the message queue.
 * The specified message is added to the queue, and the semaphore is posted
 * to indicate the availability of a message.
 *
 * @param msg The Message to enqueue.
 */
void MessageQueue::enqueue(Message *msg) {
  // Guard the mutex for thread safety
  Guard guard(m_lock);

  // Put the specified message on the queue
  m_messages.push_back(msg);

  // Post to the semaphore to indicate that a message is available
  sem_post(&m_avail);
}

/**
 * Dequeues a Message from the message queue.
 * Waits up to 1 second for a message to be available, and then removes and
 * returns the next message from the queue.
 *
 * @return A pointer to the dequeued Message, or nullptr if no message is available.
 */
Message *MessageQueue::dequeue() {
  struct timespec ts;
  
  clock_gettime(CLOCK_REALTIME, &ts);

  // Compute a time one second in the future
  ts.tv_sec += 1;

  // Call sem_timedwait to wait up to 1 second for a message
  // to be available, return nullptr if no message is available
  if (sem_timedwait(&m_avail, &ts) == -1) {
    return nullptr;
  }

  Message *msg = nullptr;

  // Guard the mutex for thread safety
  Guard g(m_lock);

  // Remove the next message from the queue and return it
  assert(!m_messages.empty());
  msg = m_messages.front();
  m_messages.pop_front();

  return msg;
}
