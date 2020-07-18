/**********************************
 * FILE NAME: MP1Node.cpp
 *
 * DESCRIPTION: Membership protocol run by this Node.
 * 				Definition of MP1Node class functions.
 **********************************/

#include "MP1Node.h"

/*
 * Note: You can change/add any functions in MP1Node.{h,cpp}
 */

/**
 * Overloaded Constructor of the MP1Node class
 * You can add new members to the class if you think it
 * is necessary for your logic to work
 */
MP1Node::MP1Node(Member *member, Params *params, EmulNet *emul, Log *log, Address *address) {
	for( int i = 0; i < 6; i++ ) {
		NULLADDR[i] = 0;
	}
	this->memberNode = member;
	this->emulNet = emul;
	this->log = log;
	this->par = params;
	this->memberNode->addr = *address;
}

/**
 * Destructor of the MP1Node class
 */
MP1Node::~MP1Node() {}

/**
 * FUNCTION NAME: recvLoop
 *
 * DESCRIPTION: This function receives message from the network and pushes into the queue
 * 				This function is called by a node to receive messages currently waiting for it
 */
int MP1Node::recvLoop() {
	  // if the node has failed don't receive any messages
    if ( memberNode->bFailed ) {
    	return false;
    }
		// otherwise, empty the buffer and push them into the queue
    else {
    	return emulNet->ENrecv(&(memberNode->addr), enqueueWrapper, NULL, 1, &(memberNode->mp1q));
    }
}

/**
 * FUNCTION NAME: enqueueWrapper
 *
 * DESCRIPTION: Enqueue the message from Emulnet into the queue
 */
int MP1Node::enqueueWrapper(void *env, char *buff, int size) {
	Queue q;
	return q.enqueue((queue<q_elt> *)env, (void *)buff, size);
}

/**
 * FUNCTION NAME: nodeStart
 *
 * DESCRIPTION: This function bootstraps the node
 * 				All initializations routines for a member.
 * 				Called by the application layer.
 */
void MP1Node::nodeStart(char *servaddrstr, short servport) {
    Address joinaddr;
    joinaddr = getJoinAddress();

    // Self booting routines
    if( initThisNode(&joinaddr) == -1 ) {
			// node failed
#ifdef DEBUGLOG
        log->LOG(&memberNode->addr, "init_thisnode failed. Exit.");
#endif
        exit(1);
    }

    if( !introduceSelfToGroup(&joinaddr) ) {
        finishUpThisNode();
				// can't introduce self to group
#ifdef DEBUGLOG
        log->LOG(&memberNode->addr, "Unable to join self to group. Exiting.");
#endif
        exit(1);
    }

		// otherwise the node has a valid address and joined the group
    return;
}

/**
 * FUNCTION NAME: initThisNode
 *
 * DESCRIPTION: Find out who I am and start up
 */
int MP1Node::initThisNode(Address *joinaddr) {
	/*
	 * This function is partially implemented and may require changes
	 */
	// pull the id and port from the array. The id is the first 4 bytes
	// and the port is the last 2
	int id = *(int*)(&memberNode->addr.addr);
	int port = *(short*)(&memberNode->addr.addr[4]);

	memberNode->bFailed = false;
	memberNode->inited = true;
	memberNode->inGroup = false;
    // node is up!
	memberNode->nnb = 0;
	memberNode->heartbeat = 0;
	memberNode->pingCounter = TFAIL;
	memberNode->timeOutCounter = -1;
  initMemberListTable(memberNode);

  return 0;
}

/**
 * FUNCTION NAME: introduceSelfToGroup
 *
 * DESCRIPTION: Join the distributed system
 */
int MP1Node::introduceSelfToGroup(Address *joinaddr) {
	MessageHdr *msg;
#ifdef DEBUGLOG
    static char s[1024];
#endif

		// the join address of a group is set to the address of the first
		// node in the group. So if these are equal you are booting up the group
    if ( 0 == memcmp((char *)&(memberNode->addr.addr), (char *)&(joinaddr->addr), sizeof(memberNode->addr.addr))) {
        // I am the group booter (first process to join the group). Boot up the group
#ifdef DEBUGLOG
        log->LOG(&memberNode->addr, "Starting up group...");
#endif
        memberNode->inGroup = true;
    }
    else {
			  // message is composed of 4 chunks: MessageHdr, followed by join
				// address, followed by one byte, followed by one heartbeat
        size_t msgsize = sizeof(MessageHdr) + sizeof(joinaddr->addr) + sizeof(long) + 1;
				// allocate space, msg is a MessageHdr pointer
        msg = (MessageHdr *) malloc(msgsize * sizeof(char));

        // create JOINREQ message: format of data is {struct Address myaddr}
				// set the message type in the first chunk of msg
        msg->msgType = JOINREQ;
				// set the node's address in the second chunk
        memcpy((char *)(msg+1), &memberNode->addr.addr, sizeof(memberNode->addr.addr));
				// set the heartbeat in the last chunk
        memcpy((char *)(msg+1) + 1 + sizeof(memberNode->addr.addr), &memberNode->heartbeat, sizeof(long));

#ifdef DEBUGLOG
        sprintf(s, "Trying to join...");
        log->LOG(&memberNode->addr, s);
#endif

        // send JOINREQ message to introducer member
        emulNet->ENsend(&memberNode->addr, joinaddr, (char *)msg, msgsize);

        free(msg);
    }

    return 1;

}

/**
 * FUNCTION NAME: finishUpThisNode
 *
 * DESCRIPTION: Wind up this node and clean up state
 */
int MP1Node::finishUpThisNode(){
	if (memberNode->inited) {
		memberNode->inGroup = false;
		memberNode->bFailed = true;
		memberNode->memberList.clear();
		memberNode->inited = false;
		// node is down!
		memberNode->nnb = 0;
		memberNode->heartbeat = 0;
		memberNode->pingCounter = 0;
		memberNode->timeOutCounter = 0;
	}
	return 0;
}

/**
 * FUNCTION NAME: nodeLoop
 *
 * DESCRIPTION: Executed periodically at each member
 * 				Check your messages in queue and perform membership protocol duties
 */
void MP1Node::nodeLoop() {
	  // if the node has failed, do nothing
    if (memberNode->bFailed) {
    	return;
    }

    // Check my messages
    checkMessages();

    // Wait until you're in the group...
    if( !memberNode->inGroup ) {
    	return;
    }

    // ...then jump in and share your responsibilites!
    nodeLoopOps();

    return;
}

/**
 * FUNCTION NAME: checkMessages
 *
 * DESCRIPTION: Check messages in the queue and call the respective message handler
 */
void MP1Node::checkMessages() {
    void *ptr;
    int size;

    // Pop waiting messages from memberNode's mp1q
    while ( !memberNode->mp1q.empty() ) {
    	ptr = memberNode->mp1q.front().elt;
    	size = memberNode->mp1q.front().size;
    	memberNode->mp1q.pop();
    	recvCallBack((void *)memberNode, (char *)ptr, size);
    }
    return;
}

/**
 * FUNCTION NAME: recvCallBack
 *
 * DESCRIPTION: Message handler for different message types
 */
bool MP1Node::recvCallBack(void *env, char *data, int size ) {
	/*
	 * Your code goes here
	 */
}

/**
 * FUNCTION NAME: nodeLoopOps
 *
 * DESCRIPTION: Check if any node hasn't responded within a timeout period and then delete
 * 				the nodes
 * 				Propagate your membership list
 */
void MP1Node::nodeLoopOps() {

	/*
	 * Your code goes here
	 */

    return;
}

/**
 * FUNCTION NAME: isNullAddress
 *
 * DESCRIPTION: Function checks if the address is NULL
 */
int MP1Node::isNullAddress(Address *addr) {
	return (memcmp(addr->addr, NULLADDR, 6) == 0 ? 1 : 0);
}

/**
 * FUNCTION NAME: getJoinAddress
 *
 * DESCRIPTION: Returns the Address of the coordinator
 */
Address MP1Node::getJoinAddress() {
    Address joinaddr;

    memset(&joinaddr, 0, sizeof(Address));
		// the id is set to 1
    *(int *)(&joinaddr.addr) = 1;
		// the port is set to 0
    *(short *)(&joinaddr.addr[4]) = 0;

    return joinaddr;
}

/**
 * FUNCTION NAME: initMemberListTable
 *
 * DESCRIPTION: Initialize the membership list
 */
void MP1Node::initMemberListTable(Member *memberNode) {
	memberNode->memberList.clear();
}

/**
 * FUNCTION NAME: printAddress
 *
 * DESCRIPTION: Print the Address
 */
void MP1Node::printAddress(Address *addr)
{
    printf("%d.%d.%d.%d:%d \n",  addr->addr[0],addr->addr[1],addr->addr[2],
                                                       addr->addr[3], *(short*)&addr->addr[4]) ;
}

void MP1Node::sendHeartbeatToPeers() {
	// construct heartbeat message
  MessageHdr *msg;
	size_t msgsize = sizeof(MessageHdr) + sizeof(memberNode->addr.addr) + sizeof(long) + 1;
	msg = (MessageHdr *) malloc(msgsize * sizeof(char));
	msg->msgType = HEARTBEAT;
	memcpy((char *)(msg+1), &memberNode->addr.addr, sizeof(memberNode->addr.addr));
	memcpy((char *)(msg+1) + 1 + sizeof(memberNode->addr.addr), &memberNode->heartbeat, sizeof(long));

	int id = *(int *)(&memberNode->addr.addr);
	int port = *(short *)(&memberNode->addr.addr[4]);

	// send heartbeat to all of your peers
	for (vector<MemberListEntry>::iterator mle = memberNode->memberList.begin(); mle != memberNode->memberList.end(); ++mle) {
    // don't send heartbeat to yourself
		if ((id != mle->id) || (port != mle->port)) {
			Address sendAddress;
			*(int *)(&(sendAddress.addr)) = mle->id;
			*(short *)(&(sendAddress.addr[4])) = mle->port;
			emulNet->ENsend(&memberNode->addr, &sendAddress, (char *) msg, msgsize);
		}
	}
	free(msg);
}

void MP1Node::updateMemberHeartbeat(Address *fromAddr, long heartbeat) {
	// loop through member list searching for the member who sent the heartbeat
	for (vector<MemberListEntry>::iterator mle = memberNode->memberList.begin(); mle != memberNode->memberList.end(); ++mle) {
		Address memberAddr;
		*(int *)(&(memberAddr.addr)) = mle->id;
		*(short *)(&(memberAddr.addr[4])) = mle->port;

		// check if it is the member who sent the heartbeat
		if (memberAddr == *fromAddr) {
			// update if the received heartbeat is later than the current heartbeat in the MemberListEntry mle
			if (heartbeat > mle->getheartbeat()) {
				mle->setheartbeat(heartbeat);
				mle->settimestamp(par->getcurrtime());
			}
			// if we have found the correct peer then we are done
			return;
		}
	}

	// otherwise, we have traversed all members, so we need to add this peer
	int fromId = *(int *)(fromAddr->addr);
	int fromPort = *(short *)(fromAddr->addr[4]);
	MemberListEntry newPeer = MemberListEntry(fromId, fromPort, heartbeat, par->getcurrtime());
	memberNode->memberList.push_back(newPeer);
	log->logNodeAdd(&memberNode->addr, fromAddr);
}
