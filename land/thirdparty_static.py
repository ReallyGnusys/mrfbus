# run as standalone on project setup to copy static third party components
# also imported by server to allow connections from outside local network
# to get these from CDNs

import requests
import os
import sys

class ThirdPartyStatic(object):
    def local_html(self, indent=4):
        s = '   <script src="'+self.static_path+'"></script>'
        return s

    def cdn_html(self, indent=4):
        s = '   <script src="'+self.cdn_url+'"></script>'
        return s


    def __init__(self, stype , label, cdn_url, force_reload=False):
        self.stype = stype
        self.label = label
        self.filename = cdn_url.split('/')[-1]
        self.static_dir  = os.path.join('static','thirdparty',self.label,self.stype)
        self.static_path = os.path.join(self.static_dir, self.filename)
        self.cdn_url = cdn_url

        if not os.path.exists(self.static_dir):
            os.makedirs(self.static_dir)
        if force_reload or not os.path.exists(self.static_path):
            print ("loading thirdparty static component from "+self.cdn_url)
            try:
                req = requests.get(self.cdn_url)
                self.cdn = req

                fp = open(self.static_path,'w+')

                if req.encoding:
                    cont = req.content.decode(req.encoding)
                else:
                    cont = req.content
                print (cont, file =fp)
                #fp.write(str(req.content))
                fp.close()
                print ("saved comp %s to %s from %s"%( label,self.static_path,self.cdn_url))

            except:
                print("ERROR loading  comp %s to %s from %s"%( label,self.static_path,self.cdn_url))
                sys.exit(-1)



class ThirdPartyStaticMgr(object):
    def __init__(self,force_reload=False): # hard coded js and css dependencies here for now

        jquery = ThirdPartyStatic('js','jquery', "https://code.jquery.com/jquery-3.2.1.slim.min.js", force_reload=force_reload)
        popper = ThirdPartyStatic('js','popper',"https://cdnjs.cloudflare.com/ajax/libs/popper.js/1.12.9/umd/popper.min.js", force_reload=force_reload)
        bootstrapjs = ThirdPartyStatic('js','bootstrap',"https://maxcdn.bootstrapcdn.com/bootstrap/4.0.0/js/bootstrap.min.js", force_reload=force_reload)
        bootstrapcss = ThirdPartyStatic('css','bootstrap',"https://maxcdn.bootstrapcdn.com/bootstrap/4.0.0/css/bootstrap.min.css", force_reload=force_reload)
        plotly =  ThirdPartyStatic('js','plotly', "https://cdn.plot.ly/plotly-latest.min.js", force_reload=force_reload)


        self.login_statics = [ bootstrapcss, jquery, popper, bootstrapjs]
        self.main_statics  = self.login_statics + [plotly]


    def slist(self,login=False):
        if login:
            sts = self.login_statics
        else:
            sts = self.main_statics
        return sts

    def local_html(self,login=False):
        s = ""
        for tps in self.slist(login=login):
            s += tps.local_html() + '\n'
        return s


    def cdn_html(self,login=False):
        s = ""
        for tps in self.slist(login=login):
            s += tps.cdn_html() + '\n'
        return s



print (__name__)
if __name__ == '__main__':
    print ("running as main")
    tm = ThirdPartyStaticMgr(force_reload=True)
