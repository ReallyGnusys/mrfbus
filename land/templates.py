public_tp = """
<html>
<head>
  <link rel="shortcut icon" href="favicon.ico" type="image/x-icon" />
  <link rel="stylesheet" type="text/css" href="/static/public/css/asa/screen.css" media="screen, projection" />
  <link rel="stylesheet" type="text/css" href="/static/public/css/asa/print.css" media="print" />
  <link rel="stylesheet" type="text/css" href="/static/public/css/asa/main.css" />
  <link rel="stylesheet" type="text/css" href="/static/public/css/asa/form.css" />
  <link rel="stylesheet" type="text/css" href="/static/public/css/asa/asa_public.css" />
  <link rel="stylesheet" type="text/css" href="/static/public/css/asa/jquery-ui-1.10.4.custom.min.css" />
  <script type="text/javascript" src="/static/public/js/jquery/jquery.js"></script> 
  <script type="text/javascript" src="/static/public/js/jquery/jquery-ui-1.10.4.custom.min.js"></script>
  <script type="text/javascript" src="/static/public/js/asa/asa_public.js"></script>
  <script type="text/javascript" src="/static/public/js/asa/asa_pstates.js"></script>
<script type="text/javascript">
  var _asa_pdata = {  
       ws_url   : " {{ ws_url }} "
      };
</script>
</head>
<body>
<div class="container" id="page">
Your organisation webchat
 <div class="asa-webchat">
   <div class="pchat-status-bar"></div>
   <div class="pchat-screen"></div>
 </div>
</div>
</body>
</html>
"""




login_tp = """<!DOCTYPE html>
<html lang="en">
  <head>
    <meta charset="utf-8">
    <meta http-equiv="X-UA-Compatible" content="IE=edge">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <!-- The above 3 meta tags *must* come first in the head; any other head content must come *after* these tags -->
    <meta name="description" content="">
    <meta name="author" content="">
    <link rel="icon" href="../../favicon.ico">

    <title>Signin Template for Bootstrap</title>

    <!-- Bootstrap core CSS -->
    <link href="static/public/bower_components/bootstrap/dist/css/bootstrap.min.css" rel="stylesheet">

    <!-- Custom styles for this template -->
    <link href="static/public/signin.css" rel="stylesheet">

  </head>

  <body>

    <div class="container">

      <form class="form-signin">
        <h2 class="form-signin-heading">Please sign in</h2>
        <label for="inputUsername" class="sr-only">Username</label>
        <input type="text" id="inputUsername" class="form-control" placeholder="User name" required autofocus>
        <label for="inputPassword" class="sr-only">Password</label>
        <input type="password" id="inputPassword" class="form-control" placeholder="Password" required>
        <div class="checkbox">
          <label>
            <input type="checkbox" value="remember-me"> Remember me
          </label>
        </div>
        <button id="login-submit-btn" class="btn btn-lg btn-primary btn-block" type="submit">Sign in</button>
      </form>

    </div> <!-- /container -->
    <!-- Placed at the end of the document so the pages load faster -->
    <script src="static/public/bower_components/jquery/dist/jquery.min.js"></script>
    <script src="static/public/bower_components/bootstrap/dist/js/bootstrap.min.js"></script>

    <script src="static/public/js/asa/asa_login.js"></script>


  </body>
</html>
"""

    

mrf_tp = """
<!DOCTYPE html>
<html lang="en">
  <head>
    <meta charset="utf-8">
    <meta http-equiv="X-UA-Compatible" content="IE=edge">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <!-- The above 3 meta tags *must* come first in the head; any other head content must come *after* these tags -->
    <meta name="description" content="">
    <meta name="author" content="">
    <link rel="icon" href="../../favicon.ico">

    <title>Fixed Top Navbar Example for Bootstrap</title>

    <!-- Bootstrap core CSS -->
    <link href="static/public/bower_components/bootstrap/dist/css/bootstrap.min.css" rel="stylesheet">


    <script type="text/javascript">
       var _mrf_sdata = {  
       ws_url   : " {{ ws_url }} ",
       sid      : {{ sob['sid'] }},
       username : "{{sob['username']}}",
       type     : "{{sob['type']}}"
      };
    </script>


  </head>

  <body>

    <div class="container">
<ul class="nav nav-pills">
    <li class="active"><a data-toggle="pill" href="#home">Home</a></li>
    <li><a data-toggle="pill" href="#temps">Temperatures</a></li>
    <li><a data-toggle="pill" href="#pumps">Pumps</a></li>
  </ul>

  <div class="tab-content">
    <div id="home" class="tab-pane fade in active">
    <h3> Mrfland running on {{host}} </h3>
    <h3>Up since {{upsince}} </h3>
   
    </div>
    <div id="temps" class="tab-pane fade">
        <h2>Temps</h2>
        <table class="table">
          <thead>
            <tr>
             <th>ID</th>
             <th>label</th>
             <th>value</th>
             <th>time</th>
            </tr>
          </thead>
          <tbody>
            <tr>
             <td>temp-000</td>
             <td>Top temperature</td>
             <td id="tempsensor-0-value">NADA</td>
             <td id="timedate-0">NADA</td>
            </tr>
            <tr>
             <td>temp-001</td>
             <td>Top temperature</td>
             <td id="tempsensor-1-value">NADA</td>
             <td id="timedate-1">NADA</td>
            </tr>
            <tr>
             <td>temp-002</td>
             <td>Top temperature</td>
             <td id="tempsensor-2-value">NADA</td>
             <td id="timedate-2">NADA</td>
            </tr>
            <tr>
             <td>temp-003</td>
             <td>Top temperature</td>
             <td id="tempsensor-3-value">NADA</td>
             <td id="timedate-3">NADA</td>
            </tr>
          </tbody>
         </table>
    </div>
    <div id="pumps" class="tab-pane fade">
        <h2>Pumps</h2>
        <table class="table">
          <thead>
            <tr>
             <th>ID</th>
             <th>label</th>
             <th>value</th>
             <th>control</th>
            </tr>
          </thead>
          <tbody>
            <tr>
             <td>pump-000</td>
             <td>Main rads</td>
             <td id="pump-0-value">NADA</td>
             <td id="pump-0-cntrl"><div class="checkbox" ><input type="checkbox" id="pump-0-cb" value="0"></div></td>
            </tr>
            <tr>
             <td>pump-001</td>
             <td>Underfloor</td>
             <td id="pump-1-value">NADA</td>
             <td id="pump-1-cntrl"><div class="checkbox" ><input type="checkbox" id="pump-1-cb" value="1"></div></td>

            </tr>
            <tr>
             <td>pump-002</td>
             <td>DHW charge 1</td>
             <td id="pump-2-value">NADA</td>
              <td id="pump-2-cntrl"><div class="checkbox" ><input type="checkbox" id="pump-2-cb" value="2"></div></td>
           </tr>
            <tr>
             <td>pump-003</td>
             <td>DHW charge 2</td>
             <td id="pump-3-value">NADA</td>
             <td id="pump-3-cntrl"><div class="checkbox" ><input type="checkbox" id="pump-3-cb" value="3"></div></td>
            </tr>
          </tbody>
         </table>

      </div>



    </div> <!-- /container -->


    <!-- Bootstrap core JavaScript
    ================================================== -->
    <!-- Placed at the end of the document so the pages load faster -->
    <script src="static/public/bower_components/jquery/dist/jquery.min.js"></script>
    <script src="static/public/bower_components/bootstrap/dist/js/bootstrap.min.js"></script>

    <script src="static/secure/js/mrf_sock.js"></script>

  </body>
</html>

"""
