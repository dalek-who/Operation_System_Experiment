#include "common.h"
#include "mbox.h"
#include "sync.h"
#include "scheduler.h"


typedef struct
{
	/* TODO */
  char message[MAX_MESSAGE_LENGTH];
} Message;

typedef struct
{
	/* TODO */
    semaphore_t used;
    semaphore_t not_used;
    semaphore_t mutex;

    char name[MBOX_NAME_LENGTH+1];
    Message box[MAX_MBOX_LENGTH];
    
    int user_number;
    
    //node_t producer_wait_queue;
    //node_t costumer_wait_queue;
    
    int write_index;
    int read_index;

} MessageBox;


static MessageBox MessageBoxen[MAX_MBOXEN];
lock_t BoxLock;

/* Perform any system-startup
 * initialization for the message
 * boxes.
 */

void init_one_mbox(mbox_t mbox){
    semaphore_init(&MessageBoxen[mbox].used,0);
    semaphore_init(&MessageBoxen[mbox].not_used,MAX_MBOX_LENGTH);
    semaphore_init(&MessageBoxen[mbox].mutex,1);

    MessageBoxen[mbox].name[0]='\0'; //if the first is '\0',strlen() would return 0
    MessageBoxen[mbox].user_number = 0;
    //queue_init(&MessageBoxen[mbox].producer_wait_queue);
    //queue_init(&MessageBoxen[mbox].costumer_wait_queue);
    MessageBoxen[mbox].write_index=0;
    MessageBoxen[mbox].read_index =0;
}

void init_mbox(void)
{
	/* TODO */
    int i;
    for(i=0 ; i < MAX_MBOXEN ; ++i)
        init_one_mbox(i);

}

/* Opens the boxes named 'name', or
 * creates a new message box if it
 * doesn't already exist.
 * A message box is a bounded buffer
 * which holds up to MAX_MBOX_LENGTH items.
 * If it fails because the message
 * box table is full, it will return -1.
 * Otherwise, it returns a message box
 * id.
 */
mbox_t do_mbox_open(const char *name)
{
  (void)name;
	/* TODO */
  int i,j;
  for(i=0 ; i<MAX_MBOXEN ; ++i){
      if( same_string(name , MessageBoxen[i].name) ){
          current_running->boxes[i] = TRUE;
          ++MessageBoxen[i].user_number;
          return i;
      }
  }

  for(i=0; i<MAX_MBOXEN; ++i){
      if(strlen(MessageBoxen[i].name) == 0){
          bcopy(name,MessageBoxen[i].name,strlen(name));
          current_running->boxes[i] = TRUE;
          ++MessageBoxen[i].user_number;
          return i;
      }
  }
}

/* Closes a message box
 */
void do_mbox_close(mbox_t mbox)
{
  (void)mbox;
	/* TODO */
  --MessageBoxen[mbox].user_number;
  current_running->boxes[mbox]=FALSE;
  if(MessageBoxen[mbox].user_number == 0){
      init_one_mbox(mbox);
  }
}

/* Determine if the given
 * message box is full.
 * Equivalently, determine
 * if sending to this mbox
 * would cause a process
 * to block.
 */
int do_mbox_is_full(mbox_t mbox)
{
  (void)mbox;
	/* TODO */
  if(MessageBoxen[mbox].used.value == MAX_MBOX_LENGTH)
    return 1;
  else
    return 0;
}

int do_mbox_is_empty(mbox_t mbox)
{
  (void)mbox;
  /* TODO */
  if(MessageBoxen[mbox].used.value == 0)
    return 1;
  else
    return 0;
}

/* Enqueues a message onto
 * a message box.  If the
 * message box is full, the
 * process will block until
 * it can add the item.
 * You may assume that the
 * message box ID has been
 * properly opened before this
 * call.
 * The message is 'nbytes' bytes
 * starting at offset 'msg'
 */
void do_mbox_send(mbox_t mbox, void *msg, int nbytes)
{
  (void)mbox;
  (void)msg;
  (void)nbytes;

  /* TODO */
  semaphore_down(&MessageBoxen[mbox].not_used);
  semaphore_down(&MessageBoxen[mbox].mutex);

  int write_index = MessageBoxen[mbox].write_index;
  int i;
  for(i = 0; i < nbytes && i < MAX_MESSAGE_LENGTH; i++)
      MessageBoxen[mbox].box[write_index].message[i] = *((char *)msg + i);
  MessageBoxen[mbox].write_index = (MessageBoxen[mbox].write_index + 1) % MAX_MBOX_LENGTH;
  
  semaphore_up(&MessageBoxen[mbox].mutex);
  semaphore_up(&MessageBoxen[mbox].used);

}

/* Receives a message from the
 * specified message box.  If
 * empty, the process will block
 * until it can remove an item.
 * You may assume that the
 * message box has been properly
 * opened before this call.
 * The message is copied into
 * 'msg'.  No more than
 * 'nbytes' bytes will by copied
 * into this buffer; longer
 * messages will be truncated.
 */
void do_mbox_recv(mbox_t mbox, void *msg, int nbytes)
{
  (void)mbox;
  (void)msg;
  (void)nbytes;
  /* TODO */

  semaphore_down(&MessageBoxen[mbox].used);
  semaphore_down(&MessageBoxen[mbox].mutex);

  int read_index = MessageBoxen[mbox].read_index;
  int i;
  for(i = 0; i < nbytes && i < MAX_MESSAGE_LENGTH; i++)
      *((char *)msg + i) = MessageBoxen[mbox].box[read_index].message[i];
  MessageBoxen[mbox].read_index = (MessageBoxen[mbox].read_index + 1) % MAX_MBOX_LENGTH;
  
  semaphore_up(&MessageBoxen[mbox].mutex);
  semaphore_up(&MessageBoxen[mbox].not_used);

}


