var fs = require('fs');
routes_list = function()
{
  data={};
}

function process(path, res,config)
{
console.log(path);
    if (path == '/config')
    {
      res.writeHead(200);
      res.end(JSON.stringify(config));
      
    }
    else
    {
      if (path == '/')
      {
        file_name='/dashboard.html';
      }
      else 
      {
        file_name=path
      }
      file_path =__dirname + "/twitter_bootstrap_admin" + file_name;
      fs.readFile(file_path,
        function (err, data) {
          if (err) {
            res.writeHead(500);
            res.end('Error loading index.html');
          }
          console.log("returning index");
          res.writeHead(200);
          res.end(data);
       });
     }

}
exports.process = process;
