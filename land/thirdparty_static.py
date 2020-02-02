# run as standalone on project setup to copy static third party components
# also imported by server to allow connections from outside local network
# to get these from CDNs

import requests
import os
import sys
import shutil

class ThirdPartyStatic(object):

    def script_tag(self,path):
        return '   <script src="'+path+'"></script>'
    def css_tag(self,path):
        return '   <link href="'+path+'" rel="stylesheet" type="text/css" />'

    def tag(self,path):
        if self.stype == 'js':
            return self.script_tag(path)
        else:  # FIXME check it's css or error on elsif
            return self.css_tag(path)


    def local_html(self, indent=4):
        return self.tag(self.static_path)

    def cdn_html(self, indent=4):
        return self.tag(self.cdn_url)


    def __init__(self, stype , label, cdn_url, login_only=True,force_reload=False,filename=None):
        self.stype = stype
        self.label = label

        self.login_only = login_only
        if type(filename) == type(""):
            self.filename = filename
        else:
            self.filename = cdn_url.split('/')[-1]
        self.static_dir  = os.path.join('static','thirdparty',self.label,self.stype)
        self.static_path = os.path.join(self.static_dir, self.filename)
        self.cdn_url = cdn_url

        if not os.path.exists(self.static_dir):
            os.makedirs(self.static_dir)
        if force_reload or not os.path.exists(self.static_path):
            print(("loading thirdparty static component from "+self.cdn_url))
            try:

                fp = open(self.static_path,'w+')
                if 'http' in self.cdn_url:
                    req = requests.get(self.cdn_url)
                    self.cdn = req


                    if req.encoding:
                        cont = req.content.decode(req.encoding)
                    else:
                        cont = req.content

                else: # we are CDN in last resort
                    srcpath = os.path.join(os.environ['MRFBUS_HOME'],'land/static',self.cdn_url)
                    print(("opening "+srcpath))
                    srcfp = open(srcpath,'r')

                    print(("fp is "+repr(srcfp)))
                    content = srcfp.read()

                #print (cont, file =fp)

                fp.write(cont)
                #fp.write(str(req.content))
                fp.close()
                print(("saved comp %s to %s from %s"%( label,self.static_path,self.cdn_url)))

            except:
                print(("ERROR loading  comp %s to %s from %s"%( label,self.static_path,self.cdn_url)))
                sys.exit(-1)



class ThirdPartyStaticMgr(object):
    def __init__(self,force_reload=False): # hard coded js and css dependencies here for now

        bootstrap = 3

        self.css = []
        self.js  = []
        jquery = ThirdPartyStatic('js','jquery', "https://code.jquery.com/jquery-3.2.1.min.js", login_only=True,force_reload=force_reload)

        if bootstrap == 3:
            #popper = ThirdPartyStatic('js','popper',"https://cdnjs.cloudflare.com/ajax/libs/popper.js/1.12.9/umd/popper.min.js", force_reload=force_reload)
            tpickjs = ThirdPartyStatic('js','timepicker', "https://cdnjs.cloudflare.com/ajax/libs/bootstrap-timepicker/0.5.2/js/bootstrap-timepicker.min.js", login_only=True,force_reload=force_reload)
            tpickcss = ThirdPartyStatic('css','timepicker', "https://cdnjs.cloudflare.com/ajax/libs/bootstrap-timepicker/0.5.2/css/bootstrap-timepicker.min.css", login_only=True,force_reload=force_reload)


            fonts =  ThirdPartyStatic('fonts','bootstrap',"https://maxcdn.bootstrapcdn.com/bootstrap/3.3.7/fonts/glyphicons-halflings-regular.woff2",login_only=False, force_reload=force_reload)
            bootstrapjs = ThirdPartyStatic('js','bootstrap',"https://maxcdn.bootstrapcdn.com/bootstrap/3.3.7/js/bootstrap.min.js", login_only=True, force_reload=force_reload)
            bootstrapcss = ThirdPartyStatic('css','bootstrap',"https://maxcdn.bootstrapcdn.com/bootstrap/3.3.7/css/bootstrap.min.css", login_only=True, force_reload=force_reload)

        else:
            #momentjs = ThirdPartyStatic('js','moment',"https://cdnjs.cloudflare.com/ajax/libs/moment.js/2.24.0/locale/en-gb.js", login_only=True,force_reload=force_reload, filename="moment.js")
            momentjs = ThirdPartyStatic('js','moment',"https://cdnjs.cloudflare.com/ajax/libs/moment.js/2.24.0/moment.min.js", login_only=True,force_reload=force_reload)
            tpickjs = ThirdPartyStatic('js','timepicker', "https://cdnjs.cloudflare.com/ajax/libs/tempusdominus-bootstrap-4/5.0.0-alpha14/js/tempusdominus-bootstrap-4.min.js", login_only=True,force_reload=force_reload)
            tpickcss = ThirdPartyStatic('css','timepicker', "https://cdnjs.cloudflare.com/ajax/libs/tempusdominus-bootstrap-4/5.0.0-alpha14/css/tempusdominus-bootstrap-4.min.css", login_only=True,force_reload=force_reload)
            fonts = ThirdPartyStatic('css','fonts',"https://stackpath.bootstrapcdn.com/font-awesome/4.7.0/css/font-awesome.min.css", login_only=True, force_reload=force_reload)

            #https://stackpath.bootstrapcdn.com/font-awesome/4.7.0/fonts/fontawesome-webfont.woff
            bootstrapcss = ThirdPartyStatic('css','bootstrap',"https://stackpath.bootstrapcdn.com/bootstrap/4.3.1/css/bootstrap.min.css", login_only=True, force_reload=force_reload)
            bootstrapjs = ThirdPartyStatic('js','bootstrap',"https://stackpath.bootstrapcdn.com/bootstrap/4.3.1/js/bootstrap.bundle.min.js", login_only=True, force_reload=force_reload)

        # waste loading plotly for login page
        plotly =  ThirdPartyStatic('js','plotly', "https://cdn.plot.ly/plotly-latest.min.js", login_only=False, force_reload=force_reload)
        if bootstrap == 3:
            self.login_statics = [ bootstrapcss, fonts, jquery, bootstrapjs]
            #self.login_statics = [ bootstrapcss, jquery, bootstrapjs]
        else:
            self.login_statics = [ bootstrapcss, fonts, jquery, bootstrapjs]

        if bootstrap == 3:
            self.main_statics  = self.login_statics + [ tpickcss,tpickjs,plotly]
        else:
            self.main_statics  = self.login_statics + [momentjs, tpickcss,tpickjs,plotly]



    def slist(self,login=False,css=False):
        if login:
            asts = self.login_statics
        else:
            asts = self.main_statics

        sts = []
        for st in asts:
            if st.stype == 'css' and css:
                sts.append(st)
            elif st.stype == 'js' and not css:
                sts.append(st)
        return sts

    def local_html(self,login=False,css=False):
        s = ""
        for tps in self.slist(login=login,css=css):
            s += tps.local_html() + '\n'
        return s


    def cdn_html(self,login=False,css=False):
        s = ""
        for tps in self.slist(login=login,css=css):
            s += tps.cdn_html() + '\n'
        return s

    def html(self,login=False,css=False,static_cdn=False):
        if static_cdn:
            return self.cdn_html(login=login,css=css)
        else:
            return self.local_html(login=login,css=css)


print (__name__)
if __name__ == '__main__':
    print ("running as main")
    tm = ThirdPartyStaticMgr(force_reload=True)
