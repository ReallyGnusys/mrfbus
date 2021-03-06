
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

    <title>MRFBUS login</title>

    {% raw css_html %}
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
    {% raw js_html %}

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

    <title>MRFBUS</title>


    <script type="text/javascript">
       var _mrf_sdata = {
       ws_url   : "{{ ws_url }}",
       sid      : {{ sob['sid'] }},
       username : "{{sob['username']}}",
       type     : "{{sob['type']}}"
      };

    </script>

  </head>
{% raw html_body %}
</html>

"""
