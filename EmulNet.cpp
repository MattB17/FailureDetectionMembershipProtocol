/**********************************
 * FILE NAME: EmulNet.cpp
 *
 * DESCRIPTION: Emulated Network classes definition
 **********************************/

#include "EmulNet.h"

/**
 * Constructor
 */
EmulNet::EmulNet(Params *p)
{
	//trace.funcEntry("EmulNet::EmulNet");
	int i,j;
	par = p;
	emulnet.setNextId(1);
	emulnet.settCurrBuffSize(0);
	enInited=0;
	// default the number of messages sent and received from each node at each
	// time point to 0
	for ( i = 0; i < MAX_NODES; i++ ) {
		for ( j = 0; j < MAX_TIME; j++ ) {
			sent_msgs[i][j] = 0;
			recv_msgs[i][j] = 0;
		}
	}
	//trace.funcExit("EmulNet::EmulNet", SUCCESS);
}

/**
 * Copy constructor
 */
EmulNet::EmulNet(EmulNet &anotherEmulNet) {
	int i, j;
	this->par = anotherEmulNet.par;
	this->enInited = anotherEmulNet.enInited;
	for ( i = 0; i < MAX_NODES; i++ ) {
		for ( j = 0; j < MAX_TIME; j++ ) {
			this->sent_msgs[i][j] = anotherEmulNet.sent_msgs[i][j];
			this->recv_msgs[i][j] = anotherEmulNet.recv_msgs[i][j];
		}
	}
	this->emulnet = anotherEmulNet.emulnet;
}

/**
 * Assignment operator overloading
 */
EmulNet& EmulNet::operator =(EmulNet &anotherEmulNet) {
	int i, j;
	this->par = anotherEmulNet.par;
	this->enInited = anotherEmulNet.enInited;
	for ( i = 0; i < MAX_NODES; i++ ) {
		for ( j = 0; j < MAX_TIME; j++ ) {
			this->sent_msgs[i][j] = anotherEmulNet.sent_msgs[i][j];
			this->recv_msgs[i][j] = anotherEmulNet.recv_msgs[i][j];
		}
	}
	this->emulnet = anotherEmulNet.emulnet;
	return *this;
}

/**
 * Destructor
 */
EmulNet::~EmulNet() {}

/**
 * FUNCTION NAME: ENinit
 *
 * DESCRIPTION: Init the emulnet for this node
 */
void *EmulNet::ENinit(Address *myaddr, short port) {
	// Initialize data structures for this member
	*(int *)(myaddr->addr) = emulnet.nextid++;
    *(short *)(&myaddr->addr[4]) = 0;
	return myaddr;
}

/**
 * FUNCTION NAME: ENsend
 *
 * DESCRIPTION: EmulNet send function
 *
 * PARAMETERS:
 *   myaddr: a pointer to the address from which the message was sent
 *   toaddr: a pointer to the address to which the message is sent
 *   data: a pointer to the data being sent
 *   size: the size of the data being sent
 *
 * RETURNS:
 * size
 */
int EmulNet::ENsend(Address *myaddr, Address *toaddr, char *data, int size) {
  // en_msg struct which has a int for size and Address fields to and from
	en_msg *em;
	static char temp[2048];
	int sendmsg = rand() % 100;

  // if the buffer size is exceeded or the message is too large or the drop
	// probability is above sendmsg, do nothing
	if( (emulnet.currbuffsize >= ENBUFFSIZE) || (size + (int)sizeof(en_msg) >= par->MAX_MSG_SIZE) || (par->dropmsg && sendmsg < (int) (par->MSG_DROP_PROB * 100)) ) {
		return 0;
	}

  // allocate enough space for an en_msg and size
	// assign the size variable to the em size parameter
	// so em points to an em object and em+1 points to a memory location
	// of size bytes where the data will be stored
	em = (en_msg *)malloc(sizeof(en_msg) + size);
	em->size = size;

  // copy myaddr and toaddr to the from and to fields of em respectively
	memcpy(&(em->from.addr), &(myaddr->addr), sizeof(em->from.addr));
	memcpy(&(em->to.addr), &(toaddr->addr), sizeof(em->from.addr));
	// copy the data to the next spot after em
	// we allocated space for an en_msg + size space above so we are copying
	// data into that extra space after en_msg
	memcpy(em + 1, data, size);

	emulnet.buff[emulnet.currbuffsize++] = em;

	// myaddr points to the address from which the message originated
	// so src dereferences this to get the node number that sent the message
	int src = *(int *)(myaddr->addr);
	int time = par->getcurrtime();

	assert(src <= MAX_NODES);
	assert(time < MAX_TIME);

  // increment the sent message count for the given node and current time
	sent_msgs[src][time]++;

	#ifdef DEBUGLOG
		sprintf(temp, "Sending 4+%d B msg type %d to %d.%d.%d.%d:%d ", size-4, *(int *)data, toaddr->addr[0], toaddr->addr[1], toaddr->addr[2], toaddr->addr[3], *(short *)&toaddr->addr[4]);
	#endif

	return size;
}

/**
 * FUNCTION NAME: ENsend
 *
 * DESCRIPTION: EmulNet send function
 *
 * provides functionality to directly send data as a string
 *
 * RETURNS:
 * size
 */
int EmulNet::ENsend(Address *myaddr, Address *toaddr, string data) {
	// convert data to a c style string
	char * str = (char *) malloc(data.length() * sizeof(char));
	memcpy(str, data.c_str(), data.size());
	// call the overloaded operator with the c style string and the size
	int ret = this->ENsend(myaddr, toaddr, str, (data.length() * sizeof(char)));
	free(str);
	return ret;
}

/**
 * FUNCTION NAME: ENrecv
 *
 * DESCRIPTION: EmulNet receive function
 *
 * RETURN:
 * 0
 */
int EmulNet::ENrecv(Address *myaddr, int (* enq)(void *, char *, int), struct timeval *t, int times, void *queue){
	// times is always assumed to be 1
	int i;
	char* tmp;
	int sz;
	en_msg *emsg;

  // loop through the messages in the buffer
	for( i = emulnet.currbuffsize - 1; i >= 0; i-- ) {
		// emsg points to the en_msg struct in buff at position i
		emsg = emulnet.buff[i];

    // if the two addresses are equal
		if ( 0 == strcmp(emsg->to.addr, myaddr->addr) ) {
			// sz is the size of the message
			sz = emsg->size;
			// allocate a c style string large enough to hold message
			tmp = (char *) malloc(sz * sizeof(char));
			// copy message into tmp
			// recall that when messages are placed in the buffer the en_msg is
			// placed followed by the data so emsg+1 refers to the data
			memcpy(tmp, (char *)(emsg+1), sz);

      // reduce the buffer size as the message has been dealt with
			emulnet.buff[i] = emulnet.buff[emulnet.currbuffsize-1];
			emulnet.currbuffsize--;

			(*enq)(queue, (char *)tmp, sz);

			free(emsg);

      // myaddr is a pointer to the Address of the destination node
			// so dst dereferences this pointer to get the node number
			int dst = *(int *)(myaddr->addr);
			int time = par->getcurrtime();

			assert(dst <= MAX_NODES);
			assert(time < MAX_TIME);

      // increments the received message count for the destination node at the
			// current time
			recv_msgs[dst][time]++;
		}
	}

	return 0;
}

/**
 * FUNCTION NAME: ENcleanup
 *
 * DESCRIPTION: Cleanup the EmulNet. Called exactly once at the end of the program.
 */
int EmulNet::ENcleanup() {
	emulnet.nextid=0;
	int i, j;
	int sent_total, recv_total;

	FILE* file = fopen("msgcount.log", "w+");

	// free everything in the buffer
	while(emulnet.currbuffsize > 0) {
		free(emulnet.buff[--emulnet.currbuffsize]);
	}

  // loop through peers
	for ( i = 1; i <= par->EN_GPSZ; i++ ) {
		fprintf(file, "node %3d ", i);
		sent_total = 0;
		recv_total = 0;

    // count number of messages sent and received by that peer across time
		for (j = 0; j < par->getcurrtime(); j++) {

			sent_total += sent_msgs[i][j];
			recv_total += recv_msgs[i][j];
			if (i != 67) {
				fprintf(file, " (%4d, %4d)", sent_msgs[i][j], recv_msgs[i][j]);
				if (j % 10 == 9) {
					fprintf(file, "\n         ");
				}
			}
			else {
				fprintf(file, "special %4d %4d %4d\n", j, sent_msgs[i][j], recv_msgs[i][j]);
			}
		}
		fprintf(file, "\n");
		fprintf(file, "node %3d sent_total %6u  recv_total %6u\n\n", i, sent_total, recv_total);
	}

	fclose(file);
	return 0;
}
