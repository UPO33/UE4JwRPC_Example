const WebSocket = require('ws');
const JwRPC = require('jwrpc');
const assert = require('assert');

function check(exp)
{
    if(exp){
        console.trace(exp);
        throw exp;
    }
}

/**@type {WebSocket.Server} */
let wss = null;
let gNumRequetCalled = 0;
let gNumRequetCalledAtAll = 0;
let gNumRpcHandled = 0;

setInterval(function(){

    console.log({gNumRequetCalled, gNumRpcHandled});

    gNumRpcHandled = 0;
    gNumRequetCalled = 0;

}, 3000);

function MakeMethod(func){
    return new JwRPC.MethodInfo(function(peer, params, callback){
        //console.log(`-----------${func.name}-----------`);
        //console.log(params);
        gNumRpcHandled++;

        func(peer, params, callback);

    }, false, 99999999, 1);
};

const gRpcs = {
    'notiHi' : MakeMethod(OnNotiHi),
    'reqSum' : MakeMethod(OnReqSum),
    'reqTimeout' : MakeMethod(OnReqTimeout),
    'reqError666' : MakeMethod(OnReqError666),
};
function OnNotiHi(peer, params, callback){
    assert(params === 'hi');
}
function OnReqSum(peer, params, callback){
    setTimeout(function(){
        callback(null, (params.a + params.b));
    }, Math.random() * 2);
   
}
function OnReqTimeout(peer, params, callback){
    //its a request we dont execute the callback. the client should receive timeout after a while.
}
function OnReqError666(peer, params, callback){
    setTimeout(() => {
        callback({code:666, message:'error is 666'});
    }, Math.random() * 2000);
}

function Main()
{
    const port = 3476;
    wss = new WebSocket.Server({'port' : port});
    wss.on('connection', OnConnection);
    wss.on('listening', function(){
        console.log(`listening on ${port}`);
    });
}

function OnConnection(ws){

    console.log('new connection accepted.');

    ws._conn = new JwRPC(ws, gRpcs);
    ws._conn.testCounter = 0;
    ws._conn.intervalHandles = [];

    ws.on('close', function (code, reason){
        console.log('closed', {code, reason});
    });
    
    ws.on('error', function(error){
        console.log('error', {error});
    });

    QueueTasks(ws._conn);
}

function QueueTasks(conn)
{
    const base = 1000;
    const mul = 100;

    const theFunc = setInterval;
    
    theFunc(function(){
        gNumRequetCalled++;
        conn.Request('testEcho', 'hello').then(function(result){
            assert(result === 'hello');
            
        }).catch(function(error){
            throw (error);
        });
    }, base + Math.random() * mul);
    

    theFunc(function(){
        gNumRequetCalled++;
        conn.Request('testTimeout', 'hello').then(function(result){
        }).catch(function(error){
            assert(error.code === JwRPC.Errors.Timeout.code);
        });
    }, base + Math.random() * mul);

    theFunc(function(){
        gNumRequetCalled++;
        conn.Request('testLongTime', null).then(function(result){
        }).catch(function(error){
            throw error;
        });
    }, base + Math.random() * mul);


    theFunc(function(){
        gNumRequetCalled++;
        conn.Request('testError666', {}).then(function(result){
            assert(false);
        }).catch(function(error){
            assert(error.code === 666);
        });
    }, base + Math.random() * mul);


    theFunc(function(){
        gNumRequetCalled++;
        conn.testCounter++;
        const curNum = conn.testCounter;
        conn.Request('testRequestCounter', []).then(function(result){
            assert(curNum === result);
        }).catch(function(error){
            assert(false);
        });
    }, base + Math.random() * mul);

    
    theFunc(function(){
        conn.Notify('notiHi', 'hi');
        gNumRequetCalled++;
    }, base + Math.random() * mul);
}



Main();