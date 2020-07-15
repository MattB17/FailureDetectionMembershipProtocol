# Membership Protocol

This repository implements a membership protocol as part of the assignment for the Coursera course [Cloud Computing Concepts, Part 1](https://www.coursera.org/learn/cloud-computing)

The instructions for the assignment are located in `mp1_specifications.pdf`

## Architecture

The protocol is implemented in a three layer stack. The Application layer sits at the top, followed by the peer-to-peer layer, and with the Emulated Network (EmulNet) layer sitting at the bottom. The three layers together work to make up the failure detection membership protocol.

### EmulNet

The Emulated Network layer is a simulation of a network layer. It provides functionality for initializing a network node, sending and receiving messages, and performing node cleanup.

The `EmulNet` class has four methods:
* `void* ENinit(Address *myaddr, short port)`
* `int ENsend(Address *myaddr, Address *toaddr, char *data, int size)`
* `int ENrecv(Address *myaddr, int (* enq)(void *, char *, int), struct timeval *t, int times, void *queue)`
* `int ENcleanup()`

`ENinit` initializes the peer's address with an id and port number

`ENsend` handles sending a message from the peer with address `myaddr` to the peer with `toaddr`. The message is specified by `data`, and `size` gives the number of bytes used by the message. Note that the peer with address `myaddr` is the peer calling the `ENsend` function. Specifically, the function constructs the message using the `en_msg` (emulated network message) struct and adds it to the buffer.

`ENrecv` handles receiving messages for the peer with address `myaddr`. Specifically, it empties the messages from the buffer and queues them.

`ENcleanup` is responsible for cleanup of the peer. Specifically, it frees the buffer and counts the number of messages sent and received per timepoint, outputting these to the log.

### Application

This is the top level application layer responsible for running the protocol.

The `Application` class has four main methods:
* `Address getjoinaddr(void)`
* `int run()`
* `void mp1run()`
* `void fail()`

`getjoinaddr` returns an `Address` object representing the address for the coordinating node.

`run` is responsible for running the application. It repeatedly runs the membership protocol and then fails some nodes. After the allotted time has been exceeded, it cleans up the nodes and network.

`mp1run` runs the membership protocol. For each node, if the node has already been inserted into the group and has not failed then that node receives messages from the network and queues them. Next, for each node, if it is time to introduce the node then the node is introduced to the group. Otherwise, if the node has already been introduced and has not failed then the messages in its queue are handled and it sends heartbeats.

`fail` is responsible for handling the failure of some peers. If the current time is 100 and we are in the single failure case, then one random node is failed. Otherwise, if the current time is 100, half of the nodes are failed.
