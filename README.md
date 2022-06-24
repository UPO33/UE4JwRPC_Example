# UE4JwRPC_Example
UE4 project containing several samples to show how to use [UE4JwRPC](https://github.com/UPO33/UE4JwRPC)

<br>

currently there are 2 samples :

1. SimpleChatCPP
    
    client for a simple public chat room that anyone can join by a username and starts typing. uses C++ APIs of the plugin.

1. SimpleChatBP

   Blueprint only version of the above.

<br>

### how to run server ?
the server is written in nodejs so first of all install nodejs, then run the following commands to install the dependencies and launch the server.
```
cd Server
npm install
node SimpleChatServer.js 
```
now our server should be ready and listening for client connections.
    
<br>

### how to run client samples ?
from editor or standalone build open the map you want. (Map_SimpleChatBP, Map_SimpleChatCPP, ... )



