
------------
How to run
------------
In this directory run:
>make
This compiles the server and client .0 Files

Next have two different terminal windows open:
Server Start:
>cd Server/
>ifconfig
after ifconfig write the ip address down for using the client
>./server <port>

port has has to be greater then 5000

Client start:
>cd Client/
./client <ip address from above> <same port from above>


Now you can use the programs as designed.

------------
Clean:
------------
in the same folder as the README run:
>make clean
Now the .o files will be removed