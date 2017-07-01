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

/* retiring pending appeal ..

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
*/

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


function mrf_web_update(obj){
    if (!obj.hasOwnProperty('tag')){
        console.error("mrf_web_update - no tag in ");
        console.error(obj);
        return;
    }

    if (!obj.hasOwnProperty('data')){
        console.error("mrf_web_update - no data in ");
        console.error(obj);
        return;
    }
    tag = obj.tag;
    data = obj.data;
    //console.log("got tag");
    //console.log(tag);
    sl = '.app-'+tag.app+'.tab-'+tag.tab+'.row-'+tag.row;

    for (var fld in data){
        jsl = sl + '.fld-'+fld;
        if (tag.app == 'timers'){
            console.log("tried to update select "+jsl + " with data "+data[fld]);
            
        }
        $(jsl).html(data[fld]);
            
    }
        
}



function init_app(){
    
    // init timepickers
    $(".mrfctrl_timepick").timepicker({showMeridian : false , showInputs : false});
    
    $('.mrfctrl_timepick').timepicker().on('hide.timepicker', function(e) {
        console.log('The time is ' + e.time.value);
        console.log('The hour is ' + e.time.hours);
        console.log('The minute is ' + e.time.minutes);
        console.log('The meridian is ' + e.time.meridian);

        val = {"hour": e.time.hours , "minute":e.time.minutes , "second" : 0  }
        app = $(this).attr('app');
        tab = $(this).attr('tab');
        row = $(this).attr('row');
        //fld = $(this).attr('fld');
        fld = $(this).attr('mc-fld');
        cdata = {"tab": tab , "row" : row, "fld" : fld,  "val" :val }
        console.log(" mrf cb app :"+app );
        console.log(cdata)
        
        ws.send(mrf_ccmd(app,"mrfctrl",cdata));
      });
    

    $('.mrfctrl_timepick').timepicker().on('show.timepicker', function(e) {
        
        
        console.log('Show : The time is ' + e.time.value);
        console.log('The hour is ' + e.time.hours);
        console.log('The minute is ' + e.time.minutes);
        console.log('The meridian is ' + e.time.meridian);
        $(this).timepicker('setTime', '11:11');
        val = {"hour": e.time.hours , "minute":e.time.minutes , "second" : 0  }
        app = $(this).attr('app');
        tab = $(this).attr('tab');
        row = $(this).attr('row');
        fld = $(this).attr('mc-fld');
        cdata = {"tab": tab , "row" : row, "fld" : fld,  "val" :val }
        console.log(" mrf cb app :"+app );
        console.log(cdata)
        
        ws.send(mrf_ccmd(app,"mrfctrl",cdata));
      });
    


    //checkboxes
    $(".mrfctrl_cb").change(
            function(){
                console.log(" mrf cb changed checked "+this.checked);
                if (this.checked){
                    val = 1;
                }
                else{
                    val = 0;
                }
                console.log(this)
                app = $(this).attr('app');
                tab = $(this).attr('tab');
                row = $(this).attr('row');
                fld = $(this).attr('mc-fld');
                cdata = {"tab": tab , "row" : row, "fld" : fld,  "val" :val }
                console.log(" mrf cb app :"+app );
                console.log(cdata)

                ws.send(mrf_ccmd(app,"mrfctrl",cdata));
            });        
    

}


//incoming socket command handler
function mrf_command(obj){
    //console.log("mrf_command : got");
    //console.log(obj);
    if (obj.cmd == 'web-update'){
        mrf_web_update(obj.data);

    }
    else if (obj.cmd == 'update-div'){
        mrf_update_div(obj.data);

    }
}

function init_socket(){
    console.log("init_socket");    
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
        };
        ws.onmessage = function (evt) { 
            var msg = evt.data;
            //console.log("got ws message");
            //console.log(evt.data);
	    
	    obj = ParseJsonString(evt.data);
	    if ( obj) {
		//console.log(obj);
		if(typeof obj.cmd == "string"){    
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



          
