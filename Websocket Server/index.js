const WebSocket = require('ws');

const PORT = 7000;
const wss = new WebSocket.Server({ port: PORT });

var cl = 0;

wss.on('connection', function(ws){

    console.log("A client just connected");
    cl += 1;
    console.log("Total Client: " + cl);

    ws.on('message', function(msg){
        console.log("Received message: " + msg);
        //ws.send("reply:" + msg);

        wss.clients.forEach(function(client){
            client.send("" + msg);
            
        });
    });

    ws.on('close', function(){
        console.log("A client just disconnected");
        cl -= 1;
        console.log("Total Client: " + cl);
    });

    ws.on('error', function(){
        console.log("A client just disconnected");
        cl -= 1;
        console.log("Total Client: " + cl);
    });
});

console.log((new Date()) + "\nServer is listening on port: " + PORT);