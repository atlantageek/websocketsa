
var http = require('http');
var fs = require('fs');
var socket_io = require('socket.io');
var http_routes=require('./http_routes');
var url = require('url');
var config={start: null, stop:null};
var redis = require("redis");
    client = redis.createClient();
    client.on("error", function (err) {
        console.log("Error " + err);
    });

function start(http_routing) {
  function handler (req, res) {
    var pathname = url.parse(req.url).pathname
    http_routes.process(pathname, res, config)
  }
  var app = http.createServer(handler).listen(8000);
  var start = client.get("wispy:config:start_freq", function (err,value)
    {
      config.start =parseInt(value);
    }
  );
  var stop = client.get("wispy:config:stop_freq", function (err,value)
    {
      config.stop =parseInt(value);
    }
  );


  myrand = function(socket) {
    return setInterval(function() {
      var r1, r2;
      var data = new Array();
       client.lrange('wispy',0,1, function(err, value)
       {
         var data = new Array();
         var datastr = value[0].split(',');
         for( var i=1;i<datastr.length;i++)
         {
           value=(parseInt(datastr[i]) );
           data.push(value);
         }
      
         send_data(socket,data);
       });
    }, 100);
  };
  var data = socket_io.listen(app);
  function send_data(socket,data)
  {
      //console.log(JSON.stringify(data));
      socket.emit('message', JSON.stringify(data));
  }
  data.sockets.on('connection', function(socket) {
    console.log("Got a connection.");
    myrand(socket);
  });
  
}
exports.start=start;
