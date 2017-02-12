$(document).ready(function() {
    console.log("inbox.index : typeof sock = "+ typeof sock);
    init_socket();
});


function MrfSocket(socket){
    this.set_socket(socket);
}

MrfSocket.prototype.set_socket = function(socket){
    if ( typeof socket == 'object')
        this.socket = socket;
    else
        this.socket = null;   

}

MrfSocket.prototype.send = function(obj){
    if ( this.socket){
        var jso =  JSON.stringify(obj)
        this.socket.send(jso)
    }
    
}
          


var mrf_socket = new MrfSocket();

sock = null;


function mrf_ccmd(cmd,data){
    cobj = Object();
    cobj['ccmd'] = cmd;
    cobj['data'] = data;
    jso = JSON.stringify(cobj)
    console.log("mrf_ccmd - cmd = "+cmd+" jso = "+jso)
    return jso;

}


function init_socket(){
    console.log("init_socket");    
    //app_state.state = 'wait-inbox';
    if ("WebSocket" in window) {
        ws = new WebSocket(_mrf_sdata.ws_url);
        console.log("ws constructed: typeof ws = "+typeof ws);
        sock = ws;
    }            
    if ( typeof ws == "object"){
        console.log(ws);
        ws.onopen = function() {
            console.log("websocket opened : ok");
            mrf_socket.set_socket(ws);
            ws.send(mrf_ccmd("send-inbox",{}));
        };
        ws.onmessage = function (evt) { 
            var msg = evt.data;
            console.log("got ws message");
            console.log(evt.data);
	    
	    obj = ParseJsonString(evt.data);
	    if ( obj) {
		//console.log(obj);
		if(typeof obj.cmd == "string"){    
                    if ( obj.cmd != 'datetime')
                        console.log("mrf_sock : got cmd "+obj.cmd+" calling mrf_command");
                    //console.log(obj);
		    mrf_command(obj);
                }
	    }

        };
        ws.onclose = function() { 
            console.log("Connection is closed...logging out"); 
            //window.location.href = window.location.origin + '/logout';
            
        };
    } else {
        console.log("WebSocket NOT supported by your Browser!");
    }
    


}



          
