$(document).ready(function() {
    console.log("inbox.index : typeof sock = "+ typeof sock);
    init_app();
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


function convert_obj_date(obj){
    if ((typeof obj == 'object') && (typeof obj.date == 'string'))
        obj.date = new Date(obj.date);
    return obj;

}

function convert_obj_date_name(obj,name){
    if ((typeof obj == 'object') &&  ( obj != null ) && obj.hasOwnProperty(name) &&  ( typeof obj[name] == 'string'))
        obj[name] = new Date(obj[name]);
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


function mrf_ccmd(app,cmd,data){
    cobj = Object();
    cobj['app'] = app;
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
        cdata = data[ch];
        jsel = '#tempsensor-'+ch+'-value';
        htm = ""+cdata.temperature.toFixed(3);
        $(''+jsel).html(htm);
        jsel = '#timedate-'+ch;
        htm = ""+cdata.date;
        $(''+jsel).html(htm);
    }

}

_NUM_RELAYS = 4;  // ouch
function mrf_heating_relays(data){

    for (var ch = 0 ; ch < data.length ; ch++){
        cdata = data[ch];
        console.log("rly ch "+ch+" = "+cdata);
        jsel = '#pump-'+ch+'-value';
        htm = ""+cdata;
        $(''+jsel).html(htm);
        $("#pump-"+ch+"-cb").prop("checked",cdata !=0);

        
    }

}


function init_app(){
    for (var ch = 0 ; ch < _NUM_RELAYS ; ch++){
        //channel = Number(ch)
        $("#pump-"+ch+"-cb").change(
            function(){
                console.log("cb changed checked "+this.checked);
                if (this.checked){
                    val = 1;
                }
                else{
                    val = 0;
                }
                ws.send(mrf_ccmd("heating","relay_set",{"chan": Number(this.value) , "val" :val }));
            });
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
    else if (obj.cmd == 'relays'){
        mrf_heating_relays(obj.data);  // FIXME - separate APP in js!!
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
            //ws.send(mrf_ccmd("send-inbox",{}));
        };
        ws.onmessage = function (evt) { 
            var msg = evt.data;
            //console.log("got ws message");
            //console.log(evt.data);
	    
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
            alert("server has closed connection - try reloading page");
            //window.location.href = window.location.origin + '/logout';
            
        };
    } else {
        console.log("WebSocket NOT supported by your Browser!");
    }
    


}



          
