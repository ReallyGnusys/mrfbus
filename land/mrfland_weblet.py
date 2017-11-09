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
                <div class="checkbox" >
                  <input type="checkbox" class="mrfctrl_cb" app="%s" tab="%s" row="%s" fld="%s">
                </div>
              </td>"""%(app, tab, str(row),fld)
            elif odict[fld] == '_mrf_ctrl_timepick':
                s += """
               <td>
                 <div class="app-%s tab-%s row-%s fld-%s">%s</div>
                 <i class="glyphicon glyphicon-time mrfctrl_timepick" app="%s" tab="%s" row="%s" mc-fld="%s"></i>
              </td>"""%(app, tab, str(row),fld, ival, app, tab, str(row),fld)
            elif odict[fld] == '_mrf_ctrl_butt':
                s += """
               <td>
                 <div class="app-%s tab-%s row-%s fld-%s">%s</div>
                 <button class="glyphicon %s mrfctrl_butt" app="%s" tab="%s" row="%s" mc-fld="%s"></i>
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
        return """<div class="mrfvar mrfapp-%s mrfvar-%s">%s</div>"""%(self.app,self.name, str(self.val))
    
class MrfWebletConfigVar(MrfWebletVar):
    def init(self,val, **kwargs):
        self._val = val  # just a simple value we keep here

        print "MrfWebletConfigVar init kwargs %s"%repr(kwargs)
        if self.val.__class__ == int or self.val.__class__ == float:
            if 'min_val' not in kwargs == None:
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
    def value(self):
        return self._val
    def set(self, value):
        self._val = value
    @property
    def html_ctrl(self):
        def cb_checked(self):
            if self.val:
                return 'checked'
            else:
                return ''
        if self.val.__class__ == bool:
            return """
            <div  class="mrfvar-ctrl-wrap" app="%s" name="%s">
              <input type="checkbox" class="mrfvar-ctrl mrvar-ctrl-cb" app="%s" name="%s" %s>
            </div>
            """%(self.app, self.name,self.app, self.name, cb_checked(self))
        if self.val.__class__ == int or self.val.__class__ == float:
            return """
            <div class="mrfvar-ctrl-wrap" app="%s" name="%s" >
                  <div  class="mrfvar-ctrl-wrap" app="%s" name="%s">%s</div>
                   <div class="glyphicon glyphicon-arrow-up mrfvar-ctrl-up" app="%s" name="%s" action="up">
                   <div class="glyphicon glyphicon-arrow-down mrfvar-ctrl-down" app="%s" name="%s" action="down">
            </div>"""%(self.app, self.name, self.app, self.name, repr(self.val),self.app, self.name,self.app, self.name)
        
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
        
        mrflog.info("creating weblet tag %s label %s"%(cdata['tag'],cdata['label']))
        self.rm = rm
        self.tag = cdata['tag']
        self.label = cdata['label']
        self.cdata = cdata
        self.graph_insts = 0
        self.vars = MrflandWebletVars()

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
        mrflog.warn("%s var_callback for %s value %s"%(self.__class__.__name__, name, self.vars.__dict__[name].val))
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
            setattr(self.vars, name, v)
    def has_var(self,name):
        return name in self.vars.__dict__

        
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
        if not self.vars.__dict__.has_key(vn):
            mrflog.error("%s cmd_mrfvar_ctrl no var found with name %s - data  was  %s"%(self.__class__.__name__,vn,repr(data)))
            return

        va = self.vars.__dict__[vn]
        
        if data['op'] == 'set':
            if not data.has_key('val'):
                mrflog.error("%s cmd_mrfvar_ctrl op was set but no val in data  %s"%(self.__class__.__name__,repr(data)))
                return
                
            if hasattr(va,'set'):
                
                mrflog.warn("%s cmd_mrfvar_ctrl set %s = %s"%(self.__class__.__name__,vn,repr(data['val'])))
                va.set(data['val'])
            
            
