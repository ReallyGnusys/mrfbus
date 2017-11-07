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
        if hasattr(self,'init'):
            self.init(val, **kwargs)
    @property
    def val(self):
        return self.value()
    
class MrfWebletConfigVar(MrfWebletVar):
    def init(self,val, **kwargs):
        self._val = val  # just a simple value we keep here

        print "MrfWebletConfigVar init kwargs %s"%repr(kwargs)
        if type(self.val) != bool:
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
    
    def html_ctrl(self):
        def cb_checked(self):
            if self.val:
                return 'checked'
            else:
                return ''
        if type(self.val) == bool:
            return """
            <div class="mrfvar" app="%s" name="%s" >
                  <input type="checkbox" class="mrfvar_cb" app="%s" name="%s" %s>
            </div>"""%(self.app, self.name,self.app, self.name, cb_checked(self))
        if type(self.val) == int or type(self.val) == float:
            return """
            <div class="mrfvar" app="%s" name="%s" >
                  <div  class="mrfvar_number" app="%s" name="%s">%s</div>
                   <div class="glyphicon glyphicon-arrow-up" app="%s" name="%s" action="up">
                   <div class="glyphicon glyphicon-arrow-down" app="%s" name="%s" action="down">
            </div>"""%(self.app, self.name, self.app, self.name, repr(self.val),self.app, self.name,self.app, self.name)
        return ""
        
class MrfWebletSensorVar(MrfWebletVar):
    def init(self,val, **kwargs):
        if 'field' not in kwargs:
            mrflog.error("%s no field  - got %s"%self.__class__.__name__,repr(kwargs))
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
                if vdata.has_key(citem[0]):
                    init_val = vdata[citem[0]]
                else:
                    init_val = citem[2]
                if citem[1] == bool:
                    self.add_var(citem[0],  MrfWebletConfigVar(self.tag,citem[0],init_val))
                elif citem[1] == int or citem[1] == float:
                    self.add_var(citem[0],  MrfWebletConfigVar(self.tag,citem[0],init_val, min_val= citem[3],max_val=citem[4],step=citem[5]))
                    

        if hasattr(self, 'init'):
            self.init()
        rm.weblet_register(self)  # ooer!

    def add_var(self,name,v):
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
