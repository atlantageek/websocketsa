
var sa_config={};
var markers = [null,null];
var trace_data = new Array();
var min_data = new Array();
var max_data = new Array();
          var socket = io.connect('http://192.168.100.110:8000');
          var canvas ;
          var canvas2 ;
          var canvas3 ;
          var canvas4 ;
          var ctx;
          var ctx2;
          var ctx3;
          var ctx4;
          var upper_bound = -40;
          var lower_bound = -100;
          var range = upper_bound - lower_bound;
          var playing = true;
          color_table = ["rgba(255,255,255,0.9)","rgba(255,128,0,0.9)","rgba(255,255,0,0.9)","rgba(0,255,0,0.9)","rgba(0,255,255,0.9)","rgba(0.928,255,0.9)","rgba(0,0,255,0.9)","rgba(128,0,255,0.9)","rgba(255,0,255,0.9)","rgba(255,0.928,0.9)","rgba(255,0,0,0.9)"];

function pos_to_idx(pos)
{
  var canvas = document.getElementById("mycanvas");
  freq_range = sa_config.stop - sa_config.start;
  width = canvas.width;
  index = Math.round(trace_data.length * pos / width) ;
  return index;
}

function pos_to_freq(pos)
{
  var canvas = document.getElementById("mycanvas");
  freq_range = sa_config.stop - sa_config.start;
  width = canvas.width;
  multiplier = freq_range / width;
  return multiplier * pos + sa_config.start;
}

$(function() {
canvas = document.getElementById("mycanvas");
canvas2 = document.getElementById("mycanvas2");
canvas3 = document.getElementById("mycanvas3");
canvas4 = document.getElementById("mycanvas4");
ctx = canvas.getContext("2d");
ctx2 = canvas2.getContext("2d");
ctx3 = canvas3.getContext("2d");
ctx4 = canvas4.getContext("2d");
draw_grid();
$.getJSON("/config", {}, function(data) {
    console.log(data);
    sa_config = data
    $("#start_freq").html(sa_config.start);
    $("#stop_freq").html(sa_config.stop);
  });
$('div.btn-group[data-toggle-name]').each(function () {
    var group = $(this);
    var form = group.parents('form').eq(0);
    var name = group.attr('data-toggle-name');
    var hidden = $('input[name="' + name + '"]', form);
    $('button', group).each(function () {
        $(this).on('click', function () {
            hidden.val($(this).data("toggle-value"));
        });
    });
});

 $("#mycanvas3").on('click', function (e) {
    var x=(e.pageX-$("#mycanvas3").offset().left);
    var marker = $("input[name=testOption]");
    if (marker.val() == 'marker1')
    {
      markers[0] = x;
      freq = pos_to_freq(x);
      
      $('#marker1_freq').html((freq / 1000.0) + " MHz");
    }
    else if (marker.val() == 'marker2')
    {
      markers[1] = x;
      freq = pos_to_freq(x);
      $('#marker2_freq').html((freq / 1000.0) + " MHz");
    }
     
  });

});
  function draw_grid()
  {
            ctx3.lineWidth=1;
            ctx3.strokeStyle = "rgba(99,0,0,0.2)";
            ctx3.font="20px Arial";
            var diff = canvas.height / range;
            for (bar = lower_bound; bar < upper_bound; bar += 5)
            {
              ctx3.beginPath();
              ctx3.moveTo(0,(bar - lower_bound) * diff);
              ctx3.lineTo(canvas.width,(bar - lower_bound) * diff);
              ctx3.stroke();
            ctx3.lineWidth=1;
            ctx3.strokeStyle = "rgba(99,0,0,0.2)";
              ctx3.fillText(bar.toString(),0,canvas.height - ((bar - lower_bound) * diff  ));
              ctx3.fillText(bar.toString(),canvas.width / 2 - 15,canvas.height - ((bar - lower_bound) * diff  ));
              ctx3.fillText(bar.toString(),canvas.width - 30,canvas.height - ((bar - lower_bound) * diff  ));
            }
  }
 function select_marker(marker_id)
 {
   this.id
 }
 function pause_play()
 {
            if (playing)
            {
              playing = false;
              $("#pauseplay").html('Play');
            }
            else
            { 
              playing = true;
              $("#pauseplay").html('Pause');
            }
          }
          function reset_extremes()
          {
            var tmp = max_data;
            max_data = min_data;
            min_data = tmp;
          }
          function do_range(i,data)
          {
            if (min_data.length <= i)
            {
               min_data[i]=data;
               max_data[i]=data;
            }
            else if (min_data[i] > data)
            {
               min_data[i] = data;
            }
            else if (max_data[i] < data)
            {
               max_data[i] = data;
            }
           
          }
          socket.on('message', function(data_json){
            if (!playing)
            {
              return;
            }
            ctx.clearRect(0,0,canvas.width,canvas.height);
            ctx2.clearRect(0,0,canvas.width,canvas.height);
            
            data_obj = JSON.parse(data_json);
            trace_data = data_obj.trace;
            dist = canvas.width / (trace_data.length - 1);
            hdist = canvas.height / range;
            ctx2.strokeStyle = 'green';
            ctx2.lineWidth=0;
            ctx2.beginPath();
            ctx2.moveTo(0, -1 * hdist * (min_data[0] - upper_bound) );

            //bounds
            for (var i=1; i<trace_data.length; i++)
            {
              ctx2.lineTo(i * dist, -1 * hdist * (min_data[i] - upper_bound) );
            }
            for (var i=trace_data.length - 1; i>= 0; i--)
            {
              ctx2.lineTo(i * dist, -1 * hdist * (max_data[i] - upper_bound) );
            }
            ctx2.closePath();
            ctx2.stroke();
            if ("image" in  data_obj)
            {
              ctx4.clearRect(0,0,canvas.width,canvas.height);
              var image = data_obj.image;
              for (var i= 0 ; i < image.length; i++)
              {
                for (var j=1; j<image[i].length; j++)
                {
                   if (image[i][j] != null )
                   {
                     color_idx = Math.round(Math.log(image[i][j]));
                     ctx4.strokeStyle = color_table[color_idx];
                     ctx4.fillStyle = color_table[color_idx];
                     ctx4.fillRect((j-1) * dist,  canvas.height - (i * hdist),dist/2,1);
                   }
                }
              }
              
              ctx4.stroke();
            }
            //ctx2.fillStyle="rgba(99,0,0,0.2)";
            //ctx2.fill();
            

            ctx.strokeStyle = 'red';
            ctx.beginPath();
            ctx.moveTo(0, -1 * hdist * (trace_data[0] - upper_bound) );
            for (var i=0; i<trace_data.length; i++)
            {
              //ctx.lineTo(i * dist,canvas.height - (((range ) + trace_data[i]) * hdist) );
              ctx.lineTo(i * dist, -1 * hdist * (trace_data[i] - upper_bound) );
              do_range(i,trace_data[i]);
            }
            ctx.stroke();
            // Draw Markers
           var colors=['blue','green'];
           for (var marker_index = 0;marker_index < 2; marker_index++)
           {
             if (markers[marker_index] != null)
             {
               ctx2.beginPath();
               ctx2.strokeStyle = colors[marker_index];
               ctx2.moveTo(markers[marker_index],0);
               ctx2.lineTo(markers[marker_index],canvas.height);
               ctx2.stroke();
               idx = pos_to_idx(markers[marker_index]);
               $('#marker' + (marker_index + 1) + '_level').html(trace_data[idx] + " dB");
               $('#marker' + (marker_index + 1) + '_max').html(max_data[idx] + " dB");
               $('#marker' + (marker_index + 1) + '_min').html(min_data[idx] + " dB");
             }
           }
          });
