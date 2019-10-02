$(document).ready(function() {
    console.log("inbox.index : typeof sock = "+ typeof sock);
    init_app();
    init_socket();
    //var plots = $(".mrf-graph");


    function onresize(){
        var plots = $(".mrf-graph");
        for (var idx = 0 ; idx < plots.length ; idx++){
            console.log("resizing plot "+plots[idx].getAttribute("id"));
            Plotly.Plots.resize( plots[idx]);  //.getAttribute("id")
        }


    }
    window.addEventListener('resize', onresize);


    $('#mrf-tabs a').click(function(e) {
        console.log("tab clicked");
        console.log(e);

        e.preventDefault();
        $(this).tab('show');
        //onresize();

    });


    $("ul.nav-tabs > li > a").on("shown.bs.tab", function(e) {
        var id = $(e.target).attr("href").substr(1);
        window.location.hash = id;
    });

    var hash = window.location.hash;

    console.log("trying to show tab "+hash)
    console.log('#mrf-tabs a[href="' + hash + '"]')
    $('#mrf-tabs a[href="' + hash + '"]').tab('show');

    /*
    $(document).on('shown.bs.tab',
                   function(e){
                       var target = $(e.target).attr("href") // activated tab
                       console.log("shown.bs.tab href "+target);

                       if (target == '#logout'){

                           if (window.confirm("Really logout?")){

                           }
                       }
                       onresize;
    });
*/

    onresize();


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

    if (tag.app == 'auto_graph') {
        mrf_auto_graph(tag.tab, obj.data)
        return;
    }

    if(tag.tab == 'mrfgraph') {
        //console.log("mrfgraph tag ");
        //console.log(tag);
        mrfgraph(tag.app, tag.row,  obj.data)
        return;
    }


    data = obj.data;



    if(tag.hasOwnProperty('mrfvar')){
        //console.log("var update "+tag.mrfvar+ " = "+data.val);
        sl = '.mrfapp-'+tag.app+'.mrfvar-'+tag.mrfvar;

        $(sl).html(""+data.val);

        // patch up check box

        sel = '.mrvar-ctrl-cb[app='+tag.app+'][name='+tag.mrfvar+']'
        cbs = $(sel)
        if (cbs.length > 0) {
            //console.log("testing sel "+sel)
            //console.log("matched "+cbs.length+" elements , setting checked to "+data.val)
            //cbs.prop('checked',data.val)
        }

        return;
    }
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


function mrfgraph_data_layout(gdata){

    var data = [];
    var layout = {
            yaxis: {title: 'temp'},
        };

    console.log("mrfgraph_data_layout snames follow");
    for (sn in gdata) {
        console.log(sn);
        sdata = gdata[sn];

        if ('temp' in sdata){

            console.log("this is temp");
            tsens = sn;

            data.push( {
                x : gdata[tsens].temp.ts,
                y : gdata[tsens].temp.value,
                name : tsens,
                type : 'scatter'
            });
        }
        else if ('memory' in sdata){

            console.log("this is memory");
            tsens = sn;
            layout = {
                yaxis: {title: 'memory'},
            };

            data.push( {
                x : gdata[tsens].temp.ts,
                y : gdata[tsens].temp.value,
                name : tsens,
                type : 'scatter'
            });
        }
        else if ('relay' in sdata){
            // support optional graphing or relays on RHS second Y axis
            console.log("this is relay");
            tsens = sn;
            if (!('yaxis2' in layout)){
                console.log("creating yaxis2");
                layout.yaxis2 =  {
                    title: 'relay',
                    overlaying: 'y',
                    side: 'right'
                };
            }
                data.push( {
                    x : gdata[tsens].relay.ts,
                    y : gdata[tsens].relay.value,
                    name : tsens,
                    yaxis : 'y2',
                    type : 'linear',
                    autorange : false,
                    rangemode : 'nonnegative',
                    range : [0,1],
                    type : 'scatter'
                });
        } else {

            console.log("not temp memory or relay!"+sn);
        }
    }

    return { data : data ,  layout : layout};
}



function mrfgraph(app, graph, data){
    console.log("mrfgraph app "+app+"  graph  "+graph);
    //console.log(data);

    dl = mrfgraph_data_layout(data);

    console.log("layout is ");
    console.log(dl.layout);
    console.log("graph is "+graph);
    Plotly.newPlot(graph,dl.data,dl.layout);

}




function mrf_auto_graph(label, data){
    //console.log("mrf_auto_graph "+label);
    //console.log(data);
    dbg = false


    for (var slabel in data.sensors) {


        if (!(slabel in _sensor_averages)){
            console.error("auto graph for label "+label+"  not found");
            continue;
        }

        sdata = data.sensors[slabel];

        for (var fld in sdata) {

            if (fld == 'ts') // shouldn't happen
                continue;

            if (typeof(_sensor_averages[slabel][fld]) == 'undefined'){  // skip any flds not in graphs - system sends all outputs for now
                //console.("failed to find fld  "+fld+"  in averages for "+slabel);
                continue;
            }

            //console.log("have fld "+fld);
            _sensor_averages[slabel][fld].value.push(sdata[fld]);
            //console.log("len _sensor_averages["+slabel+"]["+fld+"].value="+_sensor_averages[slabel][fld].value.length);
           // _sensor_averages[label][fld].ts.push(data['ts']);

        }
    }
    _sensor_ts.push(data['ts']);
    //console.log("len _sensor_ts="+_sensor_ts.length)
    dt = new Date(data['ts']);
    if ( dbg) {
        console.log("len is "+ _sensor_ts.length+" latest date is");
        console.log(dt);
    }


    limms = dt.getTime() - _sensor_hist_seconds * 1000;

    fd =  new Date(_sensor_ts[0]);

    while (fd.getTime() < limms ) {
        _sensor_ts.shift();
        fd =  new Date(_sensor_ts[0]);
        for (var slabel in data.sensors) {

            for (var fld in data.sensors[slabel])
                if (typeof(_sensor_averages[slabel][fld]) != 'undefined') {// skip any flds not in graphs - system sends all outputs for now

                    _sensor_averages[slabel][fld].value.shift();

                    console.log("deleted old value for "+slabel+" "+fld+" "+fd);
                }

        }
    }


    // need to update all plots
    var plots = $(".mrf-graph");
    console.log("len .mrf-graph = "+plots.length);
    for (var idx = 0 ; idx < plots.length ; idx++){
        plot = plots[idx];
        var divid = plot.getAttribute("id");
        var dl = plot_data_layout($(plot).data("sensors"));
        //console.log("trying to update plot "+divid);
        Plotly.update(divid,dl.data,dl.layout);
        //console.log("updated plot "+divid);
    }
}

function plot_data_layout(sensors){

    var data = [];
    var lhs;

    // FIXME hard coding here of numbers on LHS ( only one type - e.g. temp, memory ) - and logic on RHS ( 0 or 1 values)
    // no built in way of determining type at present, hence following
    if ('temp' in sensors)
        lhs = 'temp';
    else if('memory' in sensors)
        lhs = 'memory';
    else // default
        lhs = Object.keys(sensors)[0];

    console.log("plot_data_layout lhs = "+lhs)
    console.log(sensors)


    var layout = {
        yaxis: {title: lhs},
    };
        for (tid in sensors[lhs]) {
            tsens = sensors[lhs][tid];
            //console.log("plot_data_layout trying tsens "+tsens)
            data.push( {
                x : _sensor_averages[tsens][lhs].ts,
                y : _sensor_averages[tsens][lhs].value,
                name : tsens,
                type : 'scatter'
            });
        }
    // support optional graphing or relays on RHS second Y axis
    if (Object.keys(sensors).length > 1){
        if ('relay' in sensors)
            rhs = 'relay';
        else if('active' in sensors)
            rhs = 'active';
        else // default
            rhs = Object.keys(sensors)[1];

        layout.yaxis2 =  {
                title: rhs,
                overlaying: 'y',
                side: 'right'
        };
        for (tid in sensors[rhs]) {
                //console.log("trying relay "+tsens)
                tsens = sensors[rhs][tid];
                data.push( {
                    x : _sensor_averages[tsens][rhs].ts,
                    y : _sensor_averages[tsens][rhs].value,
                    name : tsens,
                    yaxis : 'y2',
                    type : 'linear',
                    autorange : false,
                    rangemode : 'nonnegative',
                    range : [0,1],
                    type : 'scatter'
                });
            }
        }


    /* FIXME! should be able to automate graph updates and shouldn't be copying */
    /*
    for (tid in sensors.memory) {
        tsens = sensors.memory[tid];
        //console.log("plot_data_layout trying tsens "+tsens)
        layout = {
            yaxis: {title: 'memory'},
        };
        data.push( {
            x : _sensor_averages[tsens].memory.ts,
            y : _sensor_averages[tsens].memory.value,
            name : tsens,
            type : 'scatter'
        });
        }
        */

    return { data : data ,  layout : layout};
}



function init_graphs(){

    var plots = $(".mrf-graph");

    for (var idx=0 ; idx < plots.length ; idx = idx+1 ){

        var plot = plots[idx];
        var divid =  plot.getAttribute('id')
        var sensors = $(plot).data('sensors')
        console.log("graph "+divid+" got sensors ");
        console.log(sensors)
        dl = plot_data_layout(sensors);

        /*
        console.log("graph "+divid+" got sensors ");
        console.log(sensors);
        // do temp first - expect temp always... hmpff
        var data = [];
        var layout = {
            yaxis: {title: 'temp'},
        };
        for (tid in sensors.temp) {
            tsens = sensors.temp[tid];
            console.log("trying tsens "+tsens)
            data.push( {
                x : _sensor_averages[tsens].temp.ts,
                y : _sensor_averages[tsens].temp.value,
                name : tsens,
                type : 'scatter'
            });
        }
        // support optional graphing or relays on RHS second Y axis
        if (typeof(sensors.relay) != 'undefined'){
            layout.yaxis2 =  {
                title: 'relay',
                overlaying: 'y',
                side: 'right'
            };
            for (tid in sensors.relay) {
            console.log("trying relay "+tsens)
                tsens = sensors.relay[tid];
                data.push( {
                    x : _sensor_averages[tsens].relay.ts,
                    y : _sensor_averages[tsens].relay.value,
                    name : tsens,
                    yaxis : 'y2',
                    type : 'linear',
                    autorange : false,
                    rangemode : 'nonnegative',
                    range : [0,1],
                    type : 'scatter'
                });
            }
        }
        */
        console.log("creating new plot in div "+divid+" with data");
        console.log(dl.data);

        Plotly.newPlot(divid,dl.data,dl.layout);
    }

}

function init_app(){

    // init timepickers
    $(".mrfctrl_timepick").timepicker({showMeridian : false , showInputs : false , minuteStep : 1 });

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
        val = {"hour": e.time.hours , "minute":e.time.minutes , "second" : 0  }
        app = $(this).attr('app');
        tab = $(this).attr('tab');
        row = $(this).attr('row');
        fld = $(this).attr('mc-fld');

        hval = $(".app-"+app+".tab-"+tab+".row-"+row+".fld-"+fld).html()
        console.log("hval = "+hval)
        $(this).timepicker('setTime', hval);



        cdata = {"tab": tab , "row" : row, "fld" : fld,  "val" :val }
        console.log(" mrf cb app :"+app );
        console.log(cdata)

        //ws.send(mrf_ccmd(app,"mrfctrl",cdata));
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
                cdata = {"tab": tab , "row" : row, "fld" : fld,  "val" : val }
                console.log(" mrf cb app :"+app );
                console.log(cdata)

                ws.send(mrf_ccmd(app,"mrfctrl",cdata));
            });

    // buttons

    $(".mrfctrl_butt").click(
        function(){
            app = $(this).attr('app');
            tab = $(this).attr('tab');
            row = $(this).attr('row');
            fld = $(this).attr('mc-fld');
            cdata = {"tab": tab , "row" : row, "fld" : fld,  "val" : 1 }
            console.log(" mrf butt app :"+app );
            console.log(cdata)
            ws.send(mrf_ccmd(app,"mrfctrl",cdata));
        });


    $(".mrfctrl_sel").change(
        function(){
            var value = this.value;
            var app = $(this).attr('app');
            var tab = $(this).attr('tab');
            var row = $(this).attr('row');
            var cdata = {"app" : app, "tab": tab , "row" : row , "value" : value}
            console.log("mrfctrl_sel changed");
            console.log(cdata);
            ws.send(mrf_ccmd(app,"mrfctrl",cdata));

        });
    // new style var controls
    //mrvar-ctrl-cb

    $(".mrvar-ctrl-cb").change(
            function(){
                console.log(" mrfvar cb changed : checked = "+this.checked);
                if (this.checked){
                    val = 1;
                }
                else{
                    val = 0;
                }
                console.log(this)
                app = $(this).attr('app');
                name = $(this).attr('name');
                cdata = {"app": app , "name" : name , "op" : 'set',  "val" : this.checked }
                console.log(cdata)

                ws.send(mrf_ccmd(app,"mrfvar_ctrl",cdata));
            });

    $(".mrfvar-ctrl-up").click(
            function(){
                console.log(" mrfvar up clicked");
                console.log(this)
                app = $(this).attr('app');
                name = $(this).attr('name');
                cdata = {"app": app , "name" : name , "op" : 'up' }
                console.log(cdata)

                ws.send(mrf_ccmd(app,"mrfvar_ctrl",cdata));
            });

    $(".mrfvar-ctrl-down").click(
            function(){
                console.log(" mrfvar down clicked");
                console.log(this)
                app = $(this).attr('app');
                name = $(this).attr('name');
                cdata = {"app": app , "name" : name , "op" : 'down' }
                console.log(cdata)

                ws.send(mrf_ccmd(app,"mrfvar_ctrl",cdata));
            });



    $(".mrfvar-ctrl-timepick").timepicker({showMeridian : false , showInputs : false , minuteStep : 1 });

    $('.mrfvar-ctrl-timepick').timepicker().on('hide.timepicker', function(e) {
        console.log('The time is ' + e.time.value);
        console.log('The hour is ' + e.time.hours);
        console.log('The minute is ' + e.time.minutes);
        console.log('The meridian is ' + e.time.meridian);


        val = ("0" + e.time.hours).slice(-2) +":"+("0"+e.time.minutes).slice(-2)
        console.log('Tstr is  ' + val);
        app = $(this).attr('app');
        name = $(this).attr('name');
        cdata = {"app": app , "name" : name, "op" : "set",  "val" : val }
        console.log(" mrf cb app :"+app );
        console.log(cdata)

        ws.send(mrf_ccmd(app,"mrfvar_ctrl",cdata));
      });


    $('.mrfvar-ctrl-timepick').timepicker().on('show.timepicker', function(e) {

        console.log('Show : The time is ' + e.time.value);
        console.log(e);
        app = $(this).attr('app');
        name = $(this).attr('name');

        hval = $(".mrfvar-"+name).html()
        console.log("hval = "+hval)
        $(this).timepicker('setTime', hval);

      });


    $(".mrfvar-ctrl-sel").change(
        function(){
            var val = this.value;
            var app = $(this).attr('app');
            var name = $(this).attr('name');

            var cdata = {"app": app , "name" : name, "op" : "set",  "val" : val }

            console.log("mrfvar-ctrl-sel changed");
            console.log(cdata);
            ws.send(mrf_ccmd(app,"mrfvar_ctrl",cdata));


        });





    //graphs
    init_graphs();
}


//incoming socket command handler
function mrf_command(obj){
    //console.log("mrf_command : got");
    //console.log(obj);
    if (obj.cmd == 'web-update'){
        mrf_web_update(obj.data);

    }
    else if (obj.cmd == 'update-graph'){
        mrf_update_graph(obj.data);

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
