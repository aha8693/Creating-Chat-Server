Sample README.txt


You can also test one client at a time by using netcat as follows:

```
nc localhost [portnumber]
```

You can also spawn a netcat “server” using the following commands:

```
nc -l <port>
```

where port is a number greater or equal to 1024. You would then type in the server responses yourself in the netcat terminal window after you get a client connected to the “server” following the sequence diagrams above.

You can then pretend to be a receiver by sending a rlogin request:

```
rlogin:alice
join:cafe
sendall:Message for everyone!
```
Or you can pretend to be a sender by sending a slogin request:

```
slogin:bob
join:cafe
<messages will appear here as they are sent to the room "cafe">
```
