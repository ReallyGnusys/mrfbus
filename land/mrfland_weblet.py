#!/usr/bin/env python
'''  Copyright (c) 2012-17 Gnusys Ltd

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
'''

"""
 base class for mrfland weblets - which represent a bootstrap pane

 start with just pane tag, label
 subclasses must implement pane_html() and js() method
"""

from collections import OrderedDict
from mrflog import mrflog
from mrfland import to_json
from mrf_sens import MrfSens
import re
import datetime


def tr_fn(inp , tr_dic):
    if tr_dic.has_key(inp):
        return tr_dic[inp]
    else:
        return inp

    
def MrflandObjectTable(app,tab, idict, rows, controls = [], postcontrols = [] , mask_cols = ['recd_date'], init_vals = {}, iclasses = {}, tr_hdr = {}):
    """ utility to generate a default html table to display an ordered dict """
    s = """
        <table class="table app-%s tab-%s">
          <thead>
            <tr>"""%(app,tab)

    # get rid of columns we don't want to display
        
    odict = OrderedDict()
    for fld in idict.keys():
        if fld not in mask_cols:
            odict[fld] = idict[fld]
    
    for cntrl in postcontrols:
        odict[cntrl[0]] = cntrl[1]
    s += """
             <th>%s</th>"""%(tr_fn('tag',tr_hdr))   # always need tag - this is row name
    for fld in odict.keys():
        s += """
             <th>%s</th>"""%(tr_fn(fld,tr_hdr))
    s += """
            </tr>
          </thead>
          <tbody>"""
    for row in rows:
        s += """
           <tr>"""
        s += """<td class="app-%s tab-%s row-%s ">%s</td>"""%(app, tab ,str(row),str(row))
        if init_vals.has_key(row):
            ivals = init_vals[row]
        else:
            ivals = None
        for fld in odict.keys():
            if iclasses.has_key(fld):
                cls = " %s"%iclasses[fld]
            else:
                cls = ''
            if ivals and ivals.has_key(fld):
                ival = ivals[fld]
            else:
                ival = ''
            if odict[fld] == '_mrf_ctrl_cb':
                s += """
                <td class="app-%s tab-%s row-%s fld-%s">"""%(app, tab, str(row), fld)
                s += """
                <span class="checkbox" >
                  <input type="checkbox" class="mrfctrl_cb" app="%s" tab="%s" row="%s" fld="%s">
                </span>
              </td>"""%(app, tab, str(row),fld)
            elif odict[fld] == '_mrf_ctrl_timepick':
                s += """
               <td>
                 <span class="app-%s tab-%s row-%s fld-%s">%s</span>
                 <i class="glyphicon glyphicon-time mrfctrl_timepick" app="%s" tab="%s" row="%s" mc-fld="%s"></i>
              </td>"""%(app, tab, str(row),fld, ival, app, tab, str(row),fld)
            elif odict[fld] == '_mrf_ctrl_butt':
                s += """
               <td>
                 <span class="app-%s tab-%s row-%s fld-%s">%s</span>
                 <button class="glyphicon %s mrfctrl_butt" app="%s" tab="%s" row="%s" mc-fld="%s"></button>
              </td>"""%(app, tab, str(row),fld, ival, cls, app, tab, str(row),fld)
            else:
                s += """<td class="app-%s tab-%s row-%s fld-%s">%s</td>"""%(app, tab, str(row), fld, ival)

        s += """
            </tr>"""
    s += """
           </tbody>
         </table>
"""
    return s

_tre = re.compile(r'([0-2][0-9]):([0-5][0-9])')

def parse_timestr(tstr):
    mob = _tre.match(tstr)
    if not mob:
        return None
    hour = int(mob.group(1))
    minute = int(mob.group(2))
    if hour > 24 or hour < 0 :
        return None
    if minute > 59 or minute < 0 :
        return None
    
    return datetime.time(hour=hour,minute=minute)



class MrfWebletVar(object):
    def __init__(self, app, name, val, **kwargs): # min_val=None,max_val=None,step=None):
        mrflog.warn("MrfWebletVar app %s name %s val %s"%(app,name,repr(val)))
        mrflog.warn("kwargs %s"%repr(kwargs))
        self.name = name
        self.app = app
        if kwargs.has_key('callback'):
            self._app_callback = kwargs['callback']
        else:
            self._app_callback = None
        if hasattr(self,'init'):
            self.init(val, **kwargs)
    def updated(self): #notice that value has changed - should be no need to pass anything
        if self._app_callback:
            self._app_callback(self.name)
    @property
    def webtag(self):
        return { 'app' : self.app, 'mrfvar' : self.name }
        
    @property
    def val(self):
        return self.value()
    @property
    def html(self):
        
        if self.val.__class__ == str:
            vp = self.val
        else:
            vp = to_json(self.val)  #hmpff
        return """<span class="mrfvar mrfapp-%s mrfvar-%s">%s</span>"""%(self.app,self.name, vp)
    
class MrfWebletConfigVar(MrfWebletVar):
    def init(self,val, **kwargs):
        self._val = val  # just a simple value we keep here
        print "MrfWebletConfigVar init kwargs %s"%repr(kwargs)

        self.check_tod()

        if self.tod.__class__ == datetime.time:
            if 'step' not in kwargs:
                mrflog.warn("%s no step"%self.__class__.__name__)
                self.step = parse_timestr("00:05")
            else:
                self.step = parse_timestr(kwargs['step'])
            return
        if self.val.__class__ == int or self.val.__class__ == float:
            if 'min_val' not in kwargs:
                mrflog.error("%s no min_val"%self.__class__.__name__)
                return
            if 'max_val' not in kwargs:
                mrflog.error("%s no max_val"%self.__class__.__name__)
                return
            if 'step' not in kwargs:
                mrflog.error("%s no step"%self.__class__.__name__)
                return

            min_val = kwargs['min_val']
            max_val = kwargs['max_val']
            step    = kwargs['step']
            
            if type(min_val) != type(self.val):
                mrflog.error("%s min_val wrong type for var %s"%(self.__class__.__name__,self.name))
                return
            if type(max_val) != type(self.val):
                mrflog.error("%s max_val wrong type"%self.__class__.__name__)
                return
            if type(step) != type(self.val):
                mrflog.error("%s step wrong type"%self.__class__.__name__)
                return
            
            self.max_val = max_val
            self.min_val = min_val
            self.step    = step

    def check_tod(self):  # check if self.val is tod

        if self.val.__class__ == str:
            mrflog.warn("yes it's str")
            r = parse_timestr(self.val)
            if r.__class__ == datetime.time:
                mrflog.warn("yes it's time")
            self.tod = r
        else:
            mrflog.warn("check_tod he say no - got class %s val %s"%(self.val.__class__, repr(self.val)))
            self.tod = None
        
    def value(self):
        return self._val


    def step_value(self, num):
        if self.tod.__class__ == datetime.time:  # tod is special case...hmpfff
            tdel = datetime.timedelta(seconds = self.step.hour*60*60 + self.step.minute*60)
            td = datetime.datetime.combine(datetime.datetime.today(), self.tod)

            td = td + num *tdel

            nv = td.strftime("%H:%M")
        else:
            nv = self.val + num*self.step
            
            if nv > self.max_val:
                nv = self.max_val
            if nv < self.min_val:
                nv = self.min_val

            
        self.set(nv)
 
            
        
    def set(self, value):
        if type(value) != type(self._val):
            mrflog.error("%s set value wrong type for var name %s"%(self.__class__.__name__,self.name))
            return
        if self._val != value:
            self._val = value
            self.check_tod()
            self.updated()

        
        
    @property
    def html_ctrl(self):
        def cb_checked(self):
            if self.val:
                return 'checked'
            else:
                return ''
        if self.val.__class__ == bool:
            return """
            <span  class="mrfvar-ctrl-wrap" app="%s" name="%s">
              <input type="checkbox" class="mrfvar-ctrl mrvar-ctrl-cb" app="%s" name="%s" %s>
            </span>
            """%(self.app, self.name,self.app, self.name, cb_checked(self))
        if self.val.__class__ == int or self.val.__class__ == float:
            return """
            <span class="mrfvar-ctrl-wrap" app="%s" name="%s" >
                   <button class="glyphicon glyphicon-arrow-up mrfvar-ctrl-up" app="%s" name="%s" action="up"></button>
                   <button class="glyphicon glyphicon-arrow-down mrfvar-ctrl-down" app="%s" name="%s" action="down"></button>
            </span>"""%(self.app, self.name, self.app, self.name, self.app, self.name)

        if self.val.__class__ == str and self.tod.__class__ == datetime.time:
            return """
            <span class="mrfvar-ctrl-wrap" app="%s" name="%s" >
               <i class="glyphicon glyphicon-time mrfvar-ctrl-timepick" app="%s" name="%s"></i>
            </span>"""%(self.app, self.name, self.app, self.name)

        
        
        return ""
        
class MrfWebletSensorVar(MrfWebletVar):
    def init(self,val, **kwargs):
        if 'field' not in kwargs:
            mrflog.error("%s no field  - got %s"%(self.__class__.__name__,repr(kwargs)))
            return
        if not hasattr(val,'output'):
            mrflog.error("%s val was not sensor - got %s"%self.__class__.__name__,repr(val))
            return

        field = kwargs['field']
                            
        if field not in val.output:
            mrflog.error("%s field %s not found in sensor output  - got %s"%self.__class__.__name__,field,repr(val.output))
            return
            
        self.field = field
        self.sens = val  # must be a sensor
        self.last_val = None #  need to see if particular output field has changed , not just any in output when callback fires
        self.sens.subscribe(self._sens_callback)
        
    def _sens_callback(self, label, data):
        ##mrflog.warn("_sen_callback %s self.value %s"%(label,repr(self.val)))
        if self.val != self.last_val:
            self.last_val = self.val
            self.updated()
        
    def value(self):
        return self.sens.output[self.field]
                        
        

class MrflandWebletVars(object):
    pass



class MrflandWeblet(object):  
    def __init__(self, rm, cdata, vdata={}):
        if not cdata.has_key('tag'):
            mrflog.error('weblet data does not contain tag')
            return
        if not cdata.has_key('label'):
            mrflog.error('weblet data does not contain label')
            return
        
        mrflog.warn("creating weblet cdata %s"%(repr(cdata)))
        self.rm = rm
        self.tag = cdata['tag']
        self.label = cdata['label']
        self.cdata = cdata
        self.graph_insts = 0
        self.var = MrflandWebletVars()

        if hasattr(self,'_config_'):
            for citem in self._config_:
                if vdata.has_key(citem[0]):  # can override default in weblet instantiation
                    init_val = vdata[citem[0]]
                else:
                    init_val = citem[1]

                self.add_var(citem[0],init_val, **citem[2])
                """
                if type(citem[1]) == type(False):
                    self.add_var(citem[0],  MrfWebletConfigVar(self.tag,citem[0],init_val))
                elif type(citem[1]) == type(1) or type(citem[1]) == type(1.0):
                    self.add_var(citem[0],  MrfWebletConfigVar(self.tag,citem[0],init_val, min_val= citem[3],max_val=citem[4],step=citem[5]))
                """

        if hasattr(self, 'init'):
            self.init()
        rm.weblet_register(self)  # ooer! .. but ugly
        
    def var_callback(self,name):
        mrflog.warn("%s var_callback for %s value %s"%(self.__class__.__name__, name, self.var.__dict__[name].val))
        if hasattr(self,'var_changed'):
            self.var_changed(name)
    
    def add_var(self, name, initval, **kwargs):
        v = None

        mrflog.warn("%s add_var name %s"%(self.__class__.__name__, name))
        kwargs['callback'] = self.var_callback
        
        if issubclass(initval.__class__ , MrfSens):
            if not kwargs.has_key('field'):
                mrflog.error("add_var initval was sensor but no field keyword")
                return
            v = MrfWebletSensorVar(self.tag, name, initval, **kwargs)
        elif initval.__class__ == int or initval.__class__ == float or initval.__class__ == bool or initval.__class__ == str: #FIXME!!
            v = MrfWebletConfigVar(self.tag, name, initval, **kwargs)
        else:
            mrflog.error("%s add_var failed name %s initval %s"%(self.__class__.__name__, name, repr(initval)))
        if v:
            setattr(self.var, name, v)
            if kwargs.has_key('graph') and kwargs['graph'] == True :  # FIXME : MUST only be sensor var
                self.rm.graph_req(v.sens.label)  # ask for managed graph 

    def has_var(self,name):
        return name in self.var.__dict__

        
    def mktag(self, tab, row):
        return { 'app' : self.tag, 'tab' : tab , 'row' : row }


    
    def pane_html_header(self):
        return '    <div id="%s" class="tab-pane fade">'%self.tag
    def pane_html_footer(self):
        return '    </div>  <!-- %s -->\n'%self.tag

    def html(self):
        self.graph_insts = 0
        s = self.pane_html_header()
        s += self.pane_html()
        s += self.pane_html_footer()
        return s

    def html_var_table(self, varlist):
        mrflog.warn("%s html_var_table varlist %s"%(self.__class__.__name__, repr(varlist)))
        s = """
        <table class="table">
          <tbody>"""
         
        for vn in varlist:

            if issubclass(vn.__class__, MrfSens):
                vname = vn.label
            else:
                vname = vn
                
            s += """
            <tr><td>"""+self.var.__dict__[vname].name+"</td><td>"+self.var.__dict__[vname].html+"</td></tr>"

        s += """
          </tbody>
        </table>"""
        return s

    def html_mc_var_table(self, hdr, rownames,  cols , ctrl=False):
        #mrflog.warn("%s html_mc_var_table hdrs %s"%(self.__class__.__name__, repr(hdrs)))
        s = """
        <table class="table">
          <thead>
          <tbody>
           <tr>"""
        s+= "<th>"+hdr+"</th>"

        for cn in cols.keys():
            s+= "<th>"+cn+"</th>"

        s += """
          </tr>
         </thead>
         <tbody>"""

        for i in range(len(rownames)):
            s += """
           <tr><td>"""+rownames[i]+"</td>"

            for cn in cols.keys():
                v = cols[cn][i]
                if ctrl:
                    if v.val.__class__ == bool:
                        s += "<td>"+v.html_ctrl+"</td>"
                    else:
                        s += "<td>"+v.html+v.html_ctrl+"</td>"
                else:
                    s += "<td>"+v.html+"</td>"
            s += "</tr>"                

        s += """
          </tbody>
        </table>"""
        return s

    
    def html_var_ctrl_table(self, varlist):


        
        s = """
        <table class="table">
          <tbody>"""
         
        for vn in varlist:
            s += """
            <tr><td>"""+self.var.__dict__[vn].name+"</td><td>"+self.var.__dict__[vn].html+"</td><td>"+self.var.__dict__[vn].html_ctrl+"</td></tr>"

        s += """
          </tbody>
        </table>"""
        return s

    def cmd(self,cmd, data=None):
        fn = 'cmd_'+cmd
        if hasattr(self, fn):
            mrflog.info( "executing weblet defined cmd %s"%cmd)
            return getattr(self,fn)(data)
        else:
            mrflog.error("%s attempt to execute undefined cmd %s"%cmd)

    def cmd_mrfvar_ctrl(self,data):
        mrflog.warn("%s cmd_mrfvar_ctrl - data  %s"%(self.__class__.__name__,repr(data)))
        
        if not data.has_key('op'):
            mrflog.error("%s cmd_mrfvar_ctrl no op specified - data  was  %s"%(self.__class__.__name__,repr(data)))
            return

        if not data.has_key('name'):
            mrflog.error("%s cmd_mrfvar_ctrl no name specified - data  was  %s"%(self.__class__.__name__,repr(data)))
            return
        vn = data['name']
        if not self.var.__dict__.has_key(vn):
            mrflog.error("%s cmd_mrfvar_ctrl no var found with name %s - data  was  %s"%(self.__class__.__name__,vn,repr(data)))
            return

        va = self.var.__dict__[vn]
        
        if data['op'] == 'set':
            if not data.has_key('val'):
                mrflog.error("%s cmd_mrfvar_ctrl op was set but no val in data  %s"%(self.__class__.__name__,repr(data)))
                return
                
            if hasattr(va,'set'):
                sval = va.val.__class__(data['val'])
                
                mrflog.warn("%s cmd_mrfvar_ctrl set %s = %s %s"%(self.__class__.__name__,vn,repr(data['val']),repr(sval)))
                
                va.set(sval)
                mrflog.warn("%s var  %s = %s"%(self.__class__.__name__,vn,repr(va.val)))

        if data['op'] == 'up':
            if hasattr(va,'step_value'):

                va.step_value(1)
                #mrflog.warn("%s cmd_mrfvar_ctrl up %s = %s"%(self.__class__.__name__,vn,repr(nv)))
                #mrflog.warn("%s var  %s = %s"%(self.__class__.__name__,vn,repr(nv)))
        if data['op'] == 'down':
            if hasattr(va,'step_value'):
                va.step_value(-1)

                #mrflog.warn("%s cmd_mrfvar_ctrl down %s = %s"%(self.__class__.__name__,vn,repr(nv)))
                #mrflog.warn("%s var  %s = %s"%(self.__class__.__name__,vn,repr(nv)))

                
            
