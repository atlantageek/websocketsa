var sa_config = {};
var markers = [null, null];
var trace_data = new Array();
var min_data = new Array();
var sa_width = 800;
var max_data = new Array();
var socket; // = io.connect('http://192.168.100.110:8000');
var canvas;
var canvas2;
var canvas3;
var canvas4;
var waterfall_ctx;
var ctx;
var ctx2;
var ctx3;
var upper_bound = -40;
var lower_bound = -100;
var range = upper_bound - lower_bound;
var playing = false;
var traces=[];
color_table = ["rgba(255,255,255,0.9)", "rgba(255,128,0,0.9)", "rgba(255,255,0,0.9)", "rgba(0,255,0,0.9)", "rgba(0,255,255,0.9)", "rgba(0.928,255,0.9)", "rgba(0,0,255,0.9)", "rgba(128,0,255,0.9)", "rgba(255,0,255,0.9)", "rgba(255,0.928,0.9)", "rgba(255,0,0,0.9)"];
cold_hot_table = [ "rgba(255,0,0)", "rgba(255,16,0)", "rgba(255,32,0)", "rgba(255,48,0)", "rgba(255,64,0)", "rgba(255,80,0)", "rgba(255,96,0)", "rgba(255,112,0)",
"rgba(255,128,0)", "rgba(255,144,0)", "rgba(255,160,0)", "rgba(255,176,0)", "rgba(255,192,0)", "rgba(255,208,0)", "rgba(255,224,0)", "rgba(255,240,0)", "rgba(255,255,0)",
"rgba(240,255,0)", "rgba(224,255,0)", "rgba(208,255,0)", "rgba(192,255,0)", "rgba(176,255,0)", "rgba(160,255,0)", "rgba(144,255,0)", "rgba(128,255,0)", "rgba(112,255,0)",
"rgba(96,255,0)", "rgba(80,255,0)", "rgba(64,255,0)", "rgba(48,255,0)", "rgba(32,255,0)", "rgba(16,255,0)"];

if (typeof MozWebSocket != "undefined") {
    socket = new MozWebSocket(get_appropriate_ws_url(), "trace-protocol");
} else {
    socket = new WebSocket(get_appropriate_ws_url(), "trace-protocol");
}
try {
    socket.onopen = function() {
        pause_play();
    }

    socket.onclose = function() {}
} catch (exception) {
    alert('<p>Error' + exception);
}



var BrowserDetect = {
    init: function() {
        this.browser = this.searchString(this.dataBrowser) || "An unknown browser";
        this.version = this.searchVersion(navigator.userAgent) || this.searchVersion(navigator.appVersion) || "an unknown version";
        this.OS = this.searchString(this.dataOS) || "an unknown OS";
    },
    searchString: function(data) {
        for (var i = 0; i < data.length; i++) {
            var dataString = data[i].string;
            var dataProp = data[i].prop;
            this.versionSearchString = data[i].versionSearch || data[i].identity;
            if (dataString) {
                if (dataString.indexOf(data[i].subString) != -1)
                    return data[i].identity;
            } else if (dataProp)
                return data[i].identity;
        }
    },
    searchVersion: function(dataString) {
        var index = dataString.indexOf(this.versionSearchString);
        if (index == -1) return;
        return parseFloat(dataString.substring(index + this.versionSearchString.length + 1));
    },
    dataBrowser: [{
        string: navigator.userAgent,
        subString: "Chrome",
        identity: "Chrome"
    }, {
        string: navigator.userAgent,
        subString: "OmniWeb",
        versionSearch: "OmniWeb/",
        identity: "OmniWeb"
    }, {
        string: navigator.vendor,
        subString: "Apple",
        identity: "Safari",
        versionSearch: "Version"
    }, {
        prop: window.opera,
        identity: "Opera",
        versionSearch: "Version"
    }, {
        string: navigator.vendor,
        subString: "iCab",
        identity: "iCab"
    }, {
        string: navigator.vendor,
        subString: "KDE",
        identity: "Konqueror"
    }, {
        string: navigator.userAgent,
        subString: "Firefox",
        identity: "Firefox"
    }, {
        string: navigator.vendor,
        subString: "Camino",
        identity: "Camino"
    }, { // for newer Netscapes (6+)
        string: navigator.userAgent,
        subString: "Netscape",
        identity: "Netscape"
    }, {
        string: navigator.userAgent,
        subString: "MSIE",
        identity: "Explorer",
        versionSearch: "MSIE"
    }, {
        string: navigator.userAgent,
        subString: "Gecko",
        identity: "Mozilla",
        versionSearch: "rv"
    }, { // for older Netscapes (4-)
        string: navigator.userAgent,
        subString: "Mozilla",
        identity: "Netscape",
        versionSearch: "Mozilla"
    }],
    dataOS: [{
        string: navigator.platform,
        subString: "Win",
        identity: "Windows"
    }, {
        string: navigator.platform,
        subString: "Mac",
        identity: "Mac"
    }, {
        string: navigator.userAgent,
        subString: "iPhone",
        identity: "iPhone/iPod"
    }, {
        string: navigator.platform,
        subString: "Linux",
        identity: "Linux"
    }]

};
BrowserDetect.init();
//document.getElementById("brow").textContent = " " + BrowserDetect.browser + " "
//        + BrowserDetect.version +" " + BrowserDetect.OS +" ";

function get_appropriate_ws_url() {
    var pcol;
    var u = document.URL;

    /*
     * We open the websocket encrypted if this page came on an
     * https:// url itself, otherwise unencrypted
     */

    if (u.substring(0, 5) == "https") {
        pcol = "wss://";
        u = u.substr(8);
    } else {
        pcol = "ws://";
        if (u.substring(0, 4) == "http")
            u = u.substr(7);
    }

    u = u.split('/');

    return pcol + u[0];
}


function pos_to_idx(pos, trace_data) {
    var canvas = document.getElementById("mycanvas");
    freq_range = sa_config.stop - sa_config.start;
    width = canvas.width;
    index = Math.round(trace_data.length * pos / width);
    return index;
}

function pos_to_freq(pos) {
    var canvas = document.getElementById("mycanvas");
    freq_range = sa_config.stop - sa_config.start;
    width = canvas.width;
    multiplier = freq_range / width;
    return multiplier * pos + sa_config.start;
}
divH = divW = 0;

function checkResize() {
    var w = $("#canvasesdiv").width();
    var h = $("#canvasesdiv").height();

    if (w != divW || h != divH) {
        $("#mycanvas").width(w);
        $("#mycanvas2").width(w);
        $("#mycanvas3").width(w);
        $("#mycanvas4").width(w);
        $("#mycanvas").height(h);
        $("#mycanvas2").height(h);
        $("#mycanvas3").height(h);
        $("#mycanvas4").height(h);
        /*what ever*/
        divH = h;
        divW = w;
    }
}

$(function() {

    divW = jQuery("canvasdiv").width;
    divH = jQuery("canvasdiv").height;
    jQuery(window).resize(checkResize);
    var timer = setInterval(checkResize, 1000);
    canvas = document.getElementById("mycanvas");
    canvas2 = document.getElementById("mycanvas2");
    canvas3 = document.getElementById("mycanvas3");
    canvas4 = document.getElementById("mycanvas4");
    ctx = canvas.getContext("2d");
    ctx2 = canvas2.getContext("2d");
    ctx3 = canvas3.getContext("2d");
    waterfall_ctx = waterfall_canvas.getContext("2d");
    draw_grid();
    $.getJSON("/config", {}, function(data) {
        console.log(data);
        sa_config = data
        $("#start_freq").html(sa_config.start);
        $("#stop_freq").html(sa_config.stop);
    });
    $('div.btn-group[data-toggle-name]').each(function() {
        var group = $(this);
        var form = group.parents('form').eq(0);
        var name = group.attr('data-toggle-name');
        var hidden = $('input[name="' + name + '"]', form);
        $('button', group).each(function() {
            $(this).on('click', function() {
                hidden.val($(this).data("toggle-value"));
            });
        });
    });

    $("#mycanvas3").on('click', function(e) {
        var x = (e.pageX - $("#mycanvas3").offset().left);
        x = x * sa_width / $("#mycanvas3").width();
        var marker = $("input[name=testOption]");
        if (marker.val() == 'marker1') {
            markers[0] = x;
            freq = pos_to_freq(x);

            $('#marker1_freq').html((Math.round(freq) / 1000.0) + " MHz");
        } else if (marker.val() == 'marker2') {
            markers[1] = x;
            freq = pos_to_freq(x);
            $('#marker2_freq').html((Math.round(freq) / 1000.0) + " MHz");
        }

    });

});

function draw_grid() {
    ctx3.lineWidth = 1;
    ctx3.strokeStyle = "rgba(99,0,0,0.2)";
    ctx3.fillStyle = "rgba(0,0,0,0.5)";
    ctx3.font = "32px Arial";
    var diff = canvas.height / range;
    for (bar = lower_bound; bar < upper_bound; bar += 5) {
        ctx3.beginPath();
        ctx3.font = "10px Georgia";
        ctx3.moveTo(0, (bar - lower_bound) * diff);
        ctx3.lineTo(canvas.width, (bar - lower_bound) * diff);
        ctx3.stroke();
        ctx3.lineWidth = 1;
        ctx3.strokeStyle = "rgba(99,0,0,0.2)";
        ctx3.fillText(bar.toString(), 0, canvas.height - ((bar - lower_bound) * diff));
        ctx3.fillText(bar.toString(), canvas.width / 2 - 15, canvas.height - ((bar - lower_bound) * diff));
        ctx3.fillText(bar.toString(), canvas.width - 30, canvas.height - ((bar - lower_bound) * diff));
    }
}

function select_marker(marker_id) {
    this.id
}

function pause_play() {
    if (playing) {
        playing = false;
        $("#pauseplay").html('Play');
    } else {
        playing = true;
        $("#pauseplay").html('Pause');
    }
}

function reset_extremes() {
    var tmp = max_data;
    max_data = min_data;
    min_data = tmp;
}

function do_range(i, data) {
    if (min_data.length <= i) {
        min_data[i] = data;
        max_data[i] = data;
    } else if (min_data[i] > data) {
        min_data[i] = data;
    } else if (max_data[i] < data) {
        max_data[i] = data;
    }

}
// dist - horizontal distance between points in trace data.
// hdist - vertical distance between points in trace data.
function draw_extremes(trace_data, dist, hdist) {
    ctx2.strokeStyle = 'green';
    ctx2.lineWidth = 0;
    ctx2.beginPath();
    ctx2.moveTo(0, -1 * hdist * (min_data[0] - upper_bound));

    //bounds
    for (var i = 1; i < trace_data.length; i++) {
        ctx2.lineTo(i * dist, -1 * hdist * (min_data[i] - upper_bound));
    }
    for (var i = trace_data.length - 1; i >= 0; i--) {
        ctx2.lineTo(i * dist, -1 * hdist * (max_data[i] - upper_bound));
    }
    ctx2.closePath();
    ctx2.stroke();
}

function draw_trace(trace_data, dist, hdist)
{
    ctx.strokeStyle = 'red';
    ctx.beginPath();
    ctx.moveTo(0, -1 * hdist * (trace_data[0] - upper_bound));
    for (var i = 0; i < trace_data.length; i++) {
        //ctx.lineTo(i * dist,canvas.height - (((range ) + trace_data[i]) * hdist) );
        ctx.lineTo(i * dist, -1 * hdist * (trace_data[i] - upper_bound));
        do_range(i, trace_data[i]);
    }
    ctx.stroke();
}
//-40 - -100
function draw_waterfall(trace_list,dist,hdist, upper_bound, lower_bound, color_list)
{
    for (var t=0; t < trace_list.length;t++)
    {
        var trace_data = trace_list[t];
        for (var idx=1;idx < trace_data.length; idx++)
        {
            var range = upper_bound - lower_bound;
            var color_idx = (trace_data[idx] - lower_bound) * 30 / range;
            waterfall_ctx.strokeStyle = color_list[color_idx];
            waterfall_ctx.beginPath();
            waterfall_ctx.moveTo( (idx-1) * dist,t);
            waterfall_ctx.lineTo( idx * dist,t);
            waterfall_ctx.stroke();
        }
    }
}

function draw_markers(trace_data, dist, hdist)
{
    var colors = ['blue', 'green'];
    for (var marker_index = 0; marker_index < 2; marker_index++) {
        if (markers[marker_index] != null) {
            ctx2.beginPath();
            ctx2.strokeStyle = colors[marker_index];
            ctx2.moveTo(markers[marker_index], 0);
            ctx2.lineTo(markers[marker_index], canvas.height);
            ctx2.stroke();
            idx = pos_to_idx(markers[marker_index], trace_data);
            $('#marker' + (marker_index + 1) + '_level').html(trace_data[idx] + " dB");
            $('#marker' + (marker_index + 1) + '_max').html(max_data[idx] + " dB");
            $('#marker' + (marker_index + 1) + '_min').html(min_data[idx] + " dB");
        }
    }
}
socket.onmessage = function(data_json) {
    if (!playing) {
        return;
    }
    ctx.clearRect(0, 0, canvas.width, canvas.height);
    ctx2.clearRect(0, 0, canvas.width, canvas.height);
    try {
        data_obj = JSON.parse(data_json.data);
    } catch (e) {
        console.log(data_json.data);
        alert(e);

    }
    var dist = canvas.width / (data_obj.trace.length - 1);
    var hdist = canvas.height / range;
    draw_extremes(data_obj.trace, dist, hdist);
    draw_trace(data_obj.trace, dist, hdist);
    draw_markers(data_obj.trace, dist, hdist);
    traces.unshift(data_obj.trace);
    if (traces > 50) {traces.pop();}
    draw_waterfall(traces, dist, hdist, upper_bound, lower_bound, cold_hot_table);


}
