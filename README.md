Operating System 2020/2021 Projects
Here are listed all proposed project ideas for this academic year.
If one would like to do a custom project that she/he designed,
it must be approved by the prof. and the tutor.

General projects guidelines:
In this section you will find generic guidelines for all projects.

General guidelines:


All projects should be hosted on github/gitlab or similars.


Each student performs commits using his own account.


We want to see the individual commits. Projects with a single commit are considered invalid.


The contribution of each member in the group to the development
should be crystal clear. Comment your sections in the source with some
initials or nickname
Example:
//gg: here we clear the list of connections and do some other stuff
List_clear(&connections);
List_pushBack(&connections, stuff_added);
//ia: I am erasing stuff because this and that
List_clear(&nodes)



Arduino guidelines:


Resources: you should use at least one interrupt, a timer and an I/O port


All projects involving Arduino should include some sort of serial
communication with the host.
The serial communication should be performed using interrupts
(polling based communication is not valid).
In this sense, you should

enable UART interrupts (both tx and rx)
whenever a UART interrupt is generated the ISR should put/get the character in two common buffers (one for rx and one for tx)
main loop reads/writes from/in this two common buffers in a deferred fashion

// rx interrupt service routine
ISR(USART0_RX_vect) {
  // read the UART register UDR0
  uint8_t c = UDR0;
  // copies values from UART register to common rx_buffer
  // ...
}
//tx interrupt service routine
ISR(USART0_UDRE_vect) {
  // copies values from common tx_buffer to UART register
  // UDR0 = ...
}
// deferred handler (called in the main loop)
uint8_t UART_getChar( /*common rx_buffer something else*/ ) {
  // waiting that something is there
  while(/*rx_buffer is empty*/);
  // reads value from common buffer
  // returns the read value
}
// deferred handler (called in the main loop)
void UART_putChar( /*common tx_buffer something else*/ ) {
  // waiting that something is there
  while(/*tx_buffer is not empty*/);
  // pops a value from tx_buffer until everthing is sent
}


The comm protocol should be binary, and data integrity should be ensured by
a checksum. Hence, if the communication between MCU and PC is performed sending strings,
the project is considered not finished.


If the project requires to setup some parameters of the server,
those should be stored permanently on the board's EEPROM.
Example: a project of Arduino includes a server
(represented by the MCU board), and a client (running on the PC).
These are two different programs. In the final version of the project,
Cutecom is not allowed for receiving data from the server
(although it can be used during the development stage).



Projects
Here is a list of pre-approved projects.
Remember, if you have an idea for a project you can propose it to us!
Each project you will find contains several tags, regarding:

group size for that project: [x1] , [x2] , ...
programming language: [C98], [C++], [C98 + C++], ...
if it can be extended as final project (aka thesis) or to cover other courses' projects: [labiagi] ...
if one wants takes a project with the [thesis] tag and does not want to present it as thesis project, the size of the group rises to 2. Still, none of the group members should present the project as bachelor thesis.



Message queues in disastrOS ---> [x1] [C98]
implement an IPC system based on message queues in disastrOS
to allow (blocking) communication between processes;
the starting point is the version of disastrOS presented in class -
available here.


Buddy allocator using bitmap ---> [x1] [C98]


the project is based on the conepts of the buddy allocator presented in class;
you implement such a system relying only on a bitbmap to store the tree, instead of the dynamic structures (trees and lists) proposed in class -
available here.


Arduino TWI communication ---> [x2] [C98]
Use two (or more arduino), one configured as master the other ones as slaves
Implement an interrupt based interaction  protocol on i2c [http://www.chrisherring.net/all/tutorial-interrupt-driven-twi-interface-for-avr-part1/.
The protocol should contain the following messages:
master -> all:                "sample"             the slaves sample all digital inputs and puts them in a struct
master -> slave[i] ->master   "get"                the slave number  sends the sampled digital inputs to the master
master -> slave[i]            "set"                the master sends to the slave  the desired output pin configuration in an internal struct, "without" applying it
master -> all:                "apply"              the slaves apply the saved pin status to the outputs
The slaves react only to interrupts. they are in halt status.
The master offers an interface to the host (PC) program to control the slaves, via UART.
All protocols are binary, packet based


arduino motor controller [x1] [C98]
If you have a DC motor with encoder, implement a closed loop motor servo using a PID controller
The arduino reads the encoder, and issues a PWM, so that the speed of the motor
measured by the encoder matches a desired speed set by the user.
The Host program allows to set a speed via uart, and the program on the arduino periodically
sends back the status (pwm, encoder position, desired encoder speed and measured encoder speed)


ardiono multi motor control [x4] [C98]
integrate project 3 and project 4. Each slave controls a motor and communicates via i2c to the master
that provides a unified interface. The event loop on the slaves is synchronized with the "apply" command.


Filesystem ---> [x2] [C98]


You should implement a very simple filesystem interface using binary files.
A stub of the expected APIs is given and no external library should be used.
Allocation should be done using linked list (LLA-FS).
Functioning is reported below:

The file system reserves the first part of the file to store:

a linked list of free blocks
linked lists of file blocks
a single global directory


Blocks are of two types

data blocks: contain "random" information (i.e. actual data of a file)
directory blocks: contain a sequence of structs of type directory_entry,
containing the blocks where the files in that folder start and
whether or not those are directory themselves.



The starting point for this project is the stub reported in the directory
simple_file_system. Note that, you should provide an
executable that tests the filesystem (and shows that it's working properly).
A sort of naive interactive shell is very appreciated - even if it is not mandatory.


Filesystem with inode ---> [x3] `[C98]

Same specification of the simple Filesystem project, however,
instead of performing block allocation and bookkeeping through linked lists,
it should be done through inodes structures (IA-FS).


Syscall in Arduino scheduler  ---> [x1] [C98]


Extend the given Arduino scheduler to support a syscall mechanism similar to the one presented in disastrOS.
Implement syscalls to write/read from the serial in a kernel buffer. The transmission/reception of data will be managed by interrupts. No ASM code is required!
The starting point for this project is the basic Arduino scheduler available
here.


Arduino MIDI keyboard ---> [x1] [C98]


Extend the simple keyboard exercise to send MIDI packets
when a key is pressed. A MIDI packet is composed by

pitch: the note to generate - represented by the key ID
time: the time between the key went from "off" to "on".

Note that, no physical keyboard is required: as we proposed in the class exercise,
keys can be simulated with 2 wires.
Still, feel free to use whatever physical button you want.
The starting point or this project is the keyboard exercise presented in class
and available here.


Private chat ---> [x2] [C98/C++11]


You should develop a private chat environment to exchange
text messages between hosts. Message encryption is optional but not required.
The project should be composed by 2 main modules:

Server: receives and stores each message in a sort of chat database.
A very naive database would consist in a User struct,
that contains everything (login credentials, chats, ...).
Each Chat structure contains the actual messages.
Client: provides a very simple interface to select a
receiver for our message and then write the content of the message.
Login is required. If a receiver is not subscribed returns an error.

The project should be tested with at least 3 users :)


Private chat with voice messages ---> [thesis] [C98/C++11]


The specs are the same of the previous projects, however, the environment
should support also voice messages - that could be send via UDP
(not encrypted).


Multi-CPU scheduler simulator  ---> [x1] [C98]
Extend the scheduler simulator proposed in class to support multi-core CPU.
Note that each core is allowed to run only one process at the time.
The starting point for this project is the CPU scheduler simulator
presented in class and available
here.