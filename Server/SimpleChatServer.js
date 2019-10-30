const WebSocket = require('ws');
const JwRPC = require('jwrpc');


/**@type {WebSocket.Server} */
let wss = null;

//all the stored messages. newly connected users will receive it
let gMessages = [];

function MakeMethod(func){
    return new JwRPC.MethodInfo(function(peer, params, callback){
        console.log(`-----------${func.name}-----------`);
        console.log(params);
        func(peer, params, callback);

    }, false, 1000, 2);
};

const gRpcs = {
    'login' : MakeMethod(OnLogin),
    'message' : MakeMethod(OnMessage),
    'typing' : MakeMethod(OnTyping),
    'getUsersState' : MakeMethod(OnGetUsersState),
    'getStoredMessages' : MakeMethod(OnGetStoredMessages),
};

function Main()
{
    const port = 3498;
    wss = new WebSocket.Server({'port' : port});
    wss.on('connection', OnConnection);
    wss.on('listening', function(){
        console.log(`listening on ${port}`);
    });
}

function OnConnection(ws){

    console.log('new connection accepted.');

    ws._conn = new JwRPC(ws, gRpcs);
    ws._conn.username = null;
    ws._conn.isTyping = false;

    ws.on('close', function (code, reason){
        console.log('closed', {code, reason});
        //tell other clients  the user left
        for(let wsc of wss.clients){
            if(!wsc || wsc === this)
                continue;

            wsc._conn.Notify('userLogout', wsc._conn.username );
        }
    });
    
    ws.on('error', function(error){
        console.log('error', {error});
    });
}



function OnLogin(peer, params, callback){
    
    if(peer.username)
        return callback({'code':0, 'message': 'already logged in'});

    //check if someone is already logged in with the specified username
    for(let wsc of wss.clients)
        if(wsc._conn.username === params.username)
            return callback({'code':1, 'message':'username taken'});
    
    //store the username, we need it in other functions
    peer.username = params.username;

    callback(null, {});

    //cast to anyone execpt ourself
    for(let wsc of wss.clients){
        if(wsc._conn !== peer){
            wsc._conn.Notify('userLogin', params.username);
        }
    }
}

/*
*/
function OnTyping(peer, params, callback){
    peer.isTyping = params;
    //tell anyone except ourself that state changed
    for(let wsc of wss.clients){
        if(wsc._conn.username && wsc._conn !== peer)
            wsc._conn.Notify('userIsTyping', {'username' : peer.username, 'isTyping': params});
    }
}
/*
gives an array containing states of online users
*/
function OnGetUsersState(peer, params, callback){
    let result = [];
    for(let wsc of wss.clients){
        if(wsc._conn.username){
            result.push({'username': wsc._conn.username, 'isTyping': wsc._conn.isTyping });
        }
    }

    callback(null, result);
}
/*
this is called when we recieve a public message from a client
*/
function OnMessage(peer, message, callback){
    const msgItem = {'username': peer.username, 'message': message };
    gMessages.push(msgItem);
    //cast to all the conncted clients
    for(let wsc of wss.clients)
        wsc._conn.Notify('message', msgItem);
}
/*
*/
function OnGetStoredMessages(peer, params, callback){
    callback(null, gMessages);
}

Main();