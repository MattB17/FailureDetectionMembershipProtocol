# Membership Protocol

This repository implements a membership protocol as part of the assignment for the Coursera course [Cloud Computing Concepts, Part 1](https://www.coursera.org/learn/cloud-computing)

The instructions for the assignment are located in `mp1_specifications.pdf`

## Architecture

The protocol is implemented in a three layer stack. The Application layer sits at the top, followed by the peer-to-peer layer, and with the Emulated Network (EmulNet) layer sitting at the bottom. The three layers together work to make up the failure detection membership protocol.

### EmulNet

The Emulated Network layer is a simulation of a network layer. It provides functionality for initializing a network node, sending and receiving messages, and performing node cleanup.

The `EmulNet` class has four methods:
* `ENinit(Address *myaddr, short port)`
* `ENsend(Address *myaddr, Address *toaddr, char *data, int size)`
* `ENrecv(Address *myaddr, int (* enq)(void *, char *, int), struct timeval *t, int times, void *queue)`
* `ENcleanup()`

`ENinit` initializes the peer's address with an id and port number

`ENsend` handles sending a message from the peer with address `myaddr` to the peer with `toaddr`. The message is specified by `data`, and `size` gives the number of bytes used by the message. Note that the peer with address `myaddr` is the peer calling the `ENsend` function. Specifically, the function constructs the message using the `en_msg` (emulated network message) struct and adds it to the buffer.

`ENrecv` handles receiving messages for the peer with address `myaddr`. Specifically, it empties the messages from the buffer and queues them.

`ENcleanup` is responsible for cleanup of the peer. Specifically, it frees the buffer and counts the number of messages sent and received per timepoint, outputting these to the log.
