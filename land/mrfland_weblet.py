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

def mrfctrl_butt_html(app,tab,row,fld,cls=""):
    return """<button class="glyphicon %s mrfctrl_butt" app="%s" tab="%s" row="%s" mc-fld="%s">%s</button>"""%\
        (cls,app,tab,row,fld,fld)


def mrfctrl_select_html(app,tab,row,fldlist,label, cls=""):
    sid = "mrfctrl_sel_%s_%s_%s"%(app,tab,row)
    s = """
<span class="form-group">
  <label>%s</label>
  <select class="form-control mrfctrl_sel"  app="%s" tab="%s" row="%s" >"""%(label,app,tab,row)
    for fld in fldlist:
        s += """
        <option>%s</option>"""%fld

    s += """
   </select>
</span>
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
        self.public = False  # set if var is placed on webpage via a method
        if kwargs.has_key('callback'):
            self._app_callback = kwargs['callback']
        else:
            self._app_callback = None
        if hasattr(self,'init'):
            self.init(val, **kwargs)

    def updated(self,wsid=None): #notice that value has changed - should be no need to pass anything
        if self._app_callback:
            self._app_callback(self.name,wsid=wsid)
    @property
    def webtag(self):
        return { 'app' : self.app, 'mrfvar' : self.name }

    @property
    def val(self):
        return self.value()
    @property
    def html(self):
        self.public = True
        if self.val.__class__ == str:
            vp = self.val
        else:
            vp = to_json(self.val)  #hmpff
        return """<span class="mrfvar mrfapp-%s mrfvar-%s">%s</span>"""%(self.app,self.name, vp)

class MrfWebletConfigVar(MrfWebletVar):
    def init(self,val, **kwargs):
        self._val = val  # just a simple value we keep here
        print "MrfWebletConfigVar init kwargs %s"%repr(kwargs)

        if kwargs.has_key('save'):  # can override default save to DB of cfg var changes
            self.save = kwargs['save']
        else:
            self.save = True


        self.check_tod()

        if self.tod.__class__ == datetime.time:
            if 'step' not in kwargs:
                mrflog.warn("%s no step"%self.__class__.__name__)
                self.step = parse_timestr("00:05")
            else:
                self.step = parse_timestr(kwargs['step'])
            return
        elif self.val.__class__ == int or self.val.__class__ == float:
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
        elif  self.val.__class__ == str and  kwargs.has_key('sel_list'):
            self.sel_list =  kwargs['sel_list']
            mrflog.warn("config var select")

    def check_tod(self):  # check if self.val is tod

        if self.val.__class__ == str:
            mrflog.warn("yes it's str")
            r = parse_timestr(self.val)
            if r.__class__ == datetime.time:
                mrflog.warn("yes it's time")
            self.tod = r
        else:
            #mrflog.warn("check_tod he say no - got class %s val %s"%(self.val.__class__, repr(self.val)))
            self.tod = None

    def value(self):
        return self._val


    def step_value(self, num):
        mrflog.warn("%s  app %s name %s num %d "%(self.__class__.__name__,self.app,self.name,num))
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



    def set(self, value, wsid=None):
        if type(value) != type(self._val):
            mrflog.error("%s set value wrong type for var name %s  - got %s expected %s"%
                         (self.__class__.__name__,self.name,value.__class__.__name__,self._val.__class__.__name__))
            return
        if self._val != value:
            self._val = value
            self.check_tod()
            self.updated(wsid=wsid)



    @property
    def html_ctrl(self):
        def cb_checked(self):
            if self.val:
                return 'checked'
            else:
                return ''
        self.public = True
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

        if self.val.__class__ == str and hasattr(self,'sel_list'):

            s = """
            <span class="form-group">
  <select class="form-control mrfvar-ctrl-sel" app="%s" name="%s" >"""%(self.app,self.name)
            for fld in self.sel_list:
                s += """
        <option>%s</option>"""%fld

            s += """
   </select>
</span>
"""
            return s




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
            mrflog.error("%s field %s not found in sensor output  - got %s"%(self.__class__.__name__,field,repr(val.output)))
            return

        self.field = field
        self.sens = val  # must be a sensor
        self.last_val = None #  need to see if particular output field has changed , not just any in output when callback fires
        self.sens.subscribe(self._sens_callback)

    def _sens_callback(self, label, data):
        if False or label == 'mem_sx02':
            mrflog.warn("_sen_callback %s self.value %s last_val %s"%(label,repr(self.val),repr(self.last_val)))
        else:
            mrflog.debug("_sen_callback %s self.value %s"%(label,repr(self.val)))
        if self.last_val == None or self.val != self.last_val:
            self.last_val = self.val
            self.updated()

    def value(self):
        return self.sens.output[self.field]



class MrflandWebletVars(object):
    def cfg_dict(self):
        keys = self.__dict__.keys()
        rv = dict()
        for key in keys:
            v = self.__dict__[key]
            if v.__class__.__name__ == 'MrfWebletConfigVar' and v.save:
                rv[key] =  self.__dict__[key].val
        return rv

    def cfg_doc(self):
        dc = dict()
        dc['data'] = self.cfg_dict()
        return dc

class MrfTimer(object):
    def __init__(self,name,en,on,off,active):
        self.name = name
        self.en   = en
        self.on   = on
        self.off  = off
        self.active = active



    def is_active(self):
        nw = datetime.datetime.now()
        nt = nw.time()

        if not self.en.val or (self.on.val == self.off.val):
            rv = False
        elif self.on.tod < self.off.tod:
            rv = (self.on.tod < nt) and (nt < self.off.tod)
        else:
            rv = (self.on.tod < nt) or ( nt < self.off.tod)

        self.active.set(rv)
        return rv



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
        #self.var = MrflandWebletVars(self.tag,self.label,self.__class__.__name__)
        self.var = MrflandWebletVars()
        self._cfg_touched = False


        self._timers = OrderedDict()

        self._relay_lut = dict()
        self._relay_shares = dict()

        if self.cdata.has_key('timers'):
            for tn in self.cdata['timers']:
                self.switch_timer(tn)



        if hasattr(self.__class__,'_config_'):
            mrflog.warn("weblet has _config_ %s"%(repr(self._config_)))

            for citem in self.__class__._config_:
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

    def _run_init(self):  # called once server is set .. e.g. timers can be set
        # set any enabled timers

        for tn in self._timers.keys():
            tmr = self._timers[tn]

            if tmr.en.val:  # set timers if enabled
                self.set_timer( tmr.on.tod , tn , 'on')
                self.set_timer( tmr.off.tod , tn , 'off')
                #self.rm.timer_reg_callback( tn , 'on' , self._timer_callback)
                #self.rm.timer_reg_callback( tn , 'off' , self._timer_callback)



        if hasattr(self,'run_init'):
            mrflog.warn( "calling run_init for weblet %s "%self.name)
            self.run_init()

    _ret = re.compile(r'(.*)_(on|off|en)')

    def restore_cfg(self,data):
        mrflog.warn("weblet %s restoring data %s"%(self.tag,repr(data)))

        for vn in data.keys():
            mrflog.warn("setting var %s to %s"%(vn,repr(data[vn])))
            dv = self.var.__dict__[vn]._val.__class__(data[vn])  # FIXME str types are unicode when they come back from DB
            self.var.__dict__[vn].set(dv)

    def tbd_graph_request_data_ready(self,wsid, graphid, data):
        self.rm.webupdate(self.graphtag(graphid),
                          {'data' : data},
                          wsid)


    def var_callback(self,name,wsid=None):
        if False and self.__class__.__name__ == 'MrfSensMemory':
            mrflog.warn("%s var_callback for %s value %s wsid %s"%(self.__class__.__name__, name, self.var.__dict__[name].val, repr(wsid)))
        else:
            mrflog.debug("%s var_callback for %s value %s wsid %s"%(self.__class__.__name__, name, self.var.__dict__[name].val, repr(wsid)))
        if self.var.__dict__[name].public:  # if set this var has been instanced in a webpage

            mrflog.debug("%s running webupdate for  %s value %s wsid %s"%(self.__class__.__name__, name, self.var.__dict__[name].val, repr(wsid)))


            self.rm.webupdate(self.var.__dict__[name].webtag,
                              { 'val' : self.var.__dict__[name].val}
            )
            self.rm.server._run_updates() # ouch



        # hmpff... see if this var belongs to a timer

        mob = MrflandWeblet._ret.match(name)
        if mob and self._timers.has_key(mob.group(1)):
            tn = mob.group(1)
            mrflog.warn("timer var changed %s  timer %s"%(name,tn))
            self._eval_timer(tn)
            tmr = self._timers[tn]
            if tmr.en.val :   # make sure timers are set
                for act in ['on','off']:
                    aval = tmr.__dict__[act]
                    self.set_timer( aval.tod , tn , act)

        if self.var.__dict__[name].__class__.__name__ == 'MrfWebletConfigVar':
            self._cfg_touched = True
        if hasattr(self,'var_changed'):
            self.var_changed(name,wsid=wsid)

    def add_var(self, name, initval, **kwargs):
        v = None

        mrflog.warn("%s add_var name %s"%(self.__class__.__name__, name))
        #kwargs['callback'] = self.var_callback

        if issubclass(initval.__class__ , MrfSens):
            if not kwargs.has_key('field'):
                mrflog.error("add_var initval was sensor but no field keyword")
                return
            mrflog.warn("%s add_var SensorVar name %s"%(self.__class__.__name__, name))

            v = MrfWebletSensorVar(self.tag, name, initval, callback=self.var_callback, **kwargs)
        elif initval.__class__ == int or initval.__class__ == float or initval.__class__ == bool or initval.__class__ == str: #FIXME!!
            mrflog.warn("%s add_var ConfigVar name %s"%(self.__class__.__name__, name))
            v = MrfWebletConfigVar(self.tag, name, initval, callback=self.var_callback, **kwargs)

        else:
            mrflog.error("%s add_var failed name %s initval %s"%(self.__class__.__name__, name, repr(initval)))
        if v:
            setattr(self.var, name, v)
            if kwargs.has_key('graph') and kwargs['graph'] == True :  # FIXME : MUST only be sensor var
                self.rm.graph_req(v.sens.label,kwargs['field'])  # ask for managed graph

    def has_var(self,name):
        return name in self.var.__dict__

    def graphtag(self, graphid):
        return { 'app' : self.tag, 'tab' : 'mrfgraph' , 'row' : graphid }

    def mktag(self, tab, row):
        return { 'app' : self.tag, 'tab' : tab , 'row' : row }

    _rer = re.compile(r'(.*)_([^\[]*)')

    def set_timer(self,tod, tag,act):  # set a timer .. just prepend our
        self.rm.set_timer(tod, self.tag, tag, act)



    def switch_timer(self, name):  # create a managed switch timer and associated vars
        mob = MrflandWeblet._rer.match(name)
        if not mob:  # expect all labels to end in _on or _off
            mrflog.error("%s switch_timer unexpected name %s"%(self.__class__.__name__, name))
            return
                 # try to find matching pump label
        pl = mob.group(1) + "_PUMP"
        mrflog.warn("trying to match timer label %s to pump %s"%(name,pl))

        smap = self.rm.senslookup(pl)
        if smap == None:
            pl = mob.group(1) + "_HEAT"
            mrflog.warn("trying to match timer label %s to heater %s"%(name,pl))
            smap = self.rm.senslookup(pl)

        if smap == None:
            pl = mob.group(1) + "_SW"
            mrflog.warn("trying to match timer label %s to switch %s"%(name,pl))
            smap = self.rm.senslookup(pl)

        if smap == None:
            mrflog.error("no matching control found for timer %s"%name)
            return

        self._relay_lut[name] = { 'smap' : smap , 'tag': pl }
        if not self._relay_shares.has_key(pl):
            self._relay_shares[pl] = dict()
        self._relay_shares[pl][name] = True  # this is just a lookup for keys, val doesn't matter

        # create vars for timer on, off, enable and active

        en = name+'_en'
        self.add_var(en, False)
        active = name+'_active'
        self.add_var(active, False,save=False)

        on = name+'_on'
        self.add_var(on, '00:00')

        off = name+'_off'
        self.add_var(off, '00:00')

        en_var = self.var.__dict__[en]
        on_var = self.var.__dict__[on]
        off_var = self.var.__dict__[off]
        active_var = self.var.__dict__[active]

        self._timers[name] = MrfTimer(name,en_var,on_var,off_var,active_var)



    def _timer_callback(self, label, act):
        mrflog.warn("%s : _timer_callback label %s act %s  "%(self.__class__.__name__,label,act))
        if self._timers.has_key(label):  # check if period timer
            mrflog.warn("%s _timer_callback : period timer label %s"%(self.__class__.__name__,label))

            self._eval_timer(label)

            ## reset timer if enabled
            tmr = self._timers[label]
            aval = tmr.__dict__[act]
            if tmr.en.val:
                self.set_timer( aval.tod , label , act)
        if hasattr(self,'timer_callback'):  # call child class callback if defined
            self.timer_callback(label,act)

    def _eval_timer(self,label):
        sensmap =  self._relay_lut[label]['smap']
        pl = self._relay_lut[label]['tag']
        relay = self.rm.sensors[pl]
        was_active = relay.req_val
        is_active = False
        for tn in self._relay_shares[pl]:

            tmr = self._timers[tn]
            is_active = is_active or tmr.is_active()

        relay.set(is_active)

    def pane_html_header(self,active=False):
        if active:
            astr = 'active'
        else:
            astr = ''
        return '    <div id="%s" class="tab-pane %s fade">'%(self.tag,astr)
    def pane_html_footer(self):
        return '    </div>  <!-- %s -->\n'%self.tag

    def html(self,active=False):
        self.graph_insts = 0
        s = self.pane_html_header(active)
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

    def html_mc_var_table(self, hdr, rownames,  cols , ctrl=False , rocols = {}):
        #mrflog.warn("%s html_mc_var_table hdrs %s"%(self.__class__.__name__, repr(hdrs)))
        s = """
        <table class="table">
          <thead>
          <tbody>
           <tr>"""
        s+= "<th>"+hdr+"</th>"

        for cn in cols.keys():
            s+= "<th>"+cn+"</th>"

        for cn in rocols.keys():
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
                v.public = True
                if ctrl:
                    if v.val.__class__ == bool:
                        s += "<td>"+v.html_ctrl+"</td>"
                    else:
                        s += "<td>"+v.html+v.html_ctrl+"</td>"
                else:
                    s += "<td>"+v.html+"</td>"

            for cn in rocols.keys():
                v = rocols[cn][i]
                v.public = True
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

        for v in varlist:
            if v.__class__ == str:  #pass var name or var ref
                vn = v
            else:
                vn = v.name


            self.var.__dict__[vn].public = True
            s += """
            <tr><td>"""+self.var.__dict__[vn].name+"</td><td>"+\
            self.var.__dict__[vn].html+"</td><td>"+\
            self.var.__dict__[vn].html_ctrl+"</td></tr>"

        s += """
          </tbody>
        </table>"""
        return s


    def timer_ctrl_table(self ,exclude_list=[],include_list=None, ro=False):
        tcols =  OrderedDict()
        tcols['enable'] =  []
        tcols['on']  = []
        tcols['off'] = []

        rocols = OrderedDict()
        rocols['active'] = []

        if include_list:
            tnames = include_list
        else:
            tnames = self._timers.keys()

        tcnames = []
        for tn in tnames:
            if tn in exclude_list:
                continue
            tcnames.append(tn)
            tmr = self._timers[tn]

            tcols['enable'].append(tmr.en)
            tcols['on'].append(tmr.on)
            tcols['off'].append(tmr.off)

            rocols['active'].append(tmr.active)

        if ro:
            del tcols['on']
            del tcols['enable']  # Filthy hack to unclutter these from auto controlled timers
            for tc in tcols.keys():
                rocols[tc] = tcols[tc]
                del tcols[tc]

        return self.html_mc_var_table('Name',tcnames, tcols ,ctrl=True,rocols=rocols)


    def cmd(self,cmd, data,wsid=None):
        fn = 'cmd_'+cmd
        if hasattr(self, fn):
            mrflog.info( "executing weblet defined cmd %s"%cmd)
            return getattr(self,fn)(data,wsid)
        else:
            mrflog.error("%s attempt to execute undefined cmd %s"%(self.__class__.__name__,cmd))

    def cmd_mrfvar_ctrl(self,data ,wsid=None):
        mrflog.warn("%s cmd_mrfvar_ctrl - data  %s wsid %s"%(self.__class__.__name__,repr(data),repr(wsid)))

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

                va.set(sval,wsid=wsid)
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
