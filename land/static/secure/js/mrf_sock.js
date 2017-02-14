$(document).ready(function() {
    console.log("inbox.index : typeof sock = "+ typeof sock);
    init_socket();
});

function ParseJsonString(str) {
    try {
        obj = JSON.parse(str);
    } catch (e) {
        return null;
    }
    return obj;
}

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
    jso = JSON.stringify(cobj);
    console.log("mrf_ccmd - cmd = "+cmd+" jso = "+jso);
    return jso;

}

function mrf_update_div(obj){
    $("#"+obj.id).html(obj.value)

}


function mrf_heating_tempsensors(data){

    for (var ch in data){
        cdata = data[ch]
        console.log("ch "+ch+" = "+cdata)
        jsel = '#tempsensor-'+ch+'-value'
        htm = ""+cdata.temperature.toFixed(3)
        console.log("trying jsel "+jsel+" with data "+htm)
        $(''+jsel).html(htm)
    }

}
//incoming socket command handler
function mrf_command(obj){
    console.log("mrf_command : got");
    console.log(obj);
    if (obj.cmd == 'update-div'){
        mrf_update_div(obj.data);

    } else if (obj.cmd == 'tempsensors'){
        mrf_heating_tempsensors(obj.data);  // FIXME - separate APP in js!!
    }
    
    
    

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



          
