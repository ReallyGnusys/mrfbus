

function login_failure(data){
    console.log("login failure");
    console.log(data);
}
function ajax_success(data){

    console.log("ajax_success");
    console.log(data);
    if (( typeof data.result != 'string') || ( data.result != 'success'))
        return login_failure(data);

    if ( typeof data.redirect != 'string')
        return login_failure(data);
    console.log("about to redirect to "+data.redirect);

    window.location.href = data.redirect;

}

function staff_login_callback(data){
    console.log("staff_login_callback");
    console.log(data);
    $.ajax({
        type: "POST",
        url: '/secure/login',
        data: data,
        success: ajax_success,
        error : function(jqXHR,status,error){
            console.log("ajax_error");
            console.log(jqXHR);
            console.log(status);
            console.log(error);
            console.log("end error");
        },
        dataType: 'json'
    });


}

$(document).ready(function() {
    console.log("asa_login  : document ready");
    //form1 = new AsaForm('login_form','.staff-login-form',fm,'Login',staff_login_callback,_sl_frm1.template,_sl_frm1.items,null,true);
    $('#login-submit-btn').on('click',
                              function(){
                                  console.log("submit on click anon function username "+$("#inputUsername").val());
                                  data = { username : $("#inputUsername").val(),
                                           password : $("#inputPassword").val()
                                         };
                                  staff_login_callback(data);

                              });
    console.log("(doc ready: form constructed");
    //var felem = "#asa-fcntrl-login_form-password"
    //$(felem).focus();

});
