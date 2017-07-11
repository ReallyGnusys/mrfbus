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
import mrflog

def MrflandObjectTable(app,tab, idict, rows, controls = [], postcontrols = [] , mask_cols = ['recd_date'], init_vals = {}):
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
             <th>%s</th>"""%("tag")   # always need tag - this is row name
    for fld in odict.keys():
        s += """
             <th>%s</th>"""%(fld)
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
            else:
                s += """<td class="app-%s tab-%s row-%s fld-%s">%s</td>"""%(app, tab, str(row), fld, ival)

        s += """
            </tr>"""
    s += """
           </tbody>
         </table>
"""
    return s


class MrflandWeblet(object):  
    def __init__(self, rm, data):
        if not data.has_key('tag'):
            mrflog.error('weblet data does not contain tag')
            return
        if not data.has_key('label'):
            mrflog.error('weblet data does not contain label')
            return
        
        mrflog.info("creating weblet tag %s label %s"%(data['tag'],data['label']))
        self.rm = rm
        self.tag = data['tag']
        self.label = data['label']
        self.data = data
        if hasattr(self, 'post_init'):
            self.post_init()
    def mktag(self, tab, row):
        return { 'app' : self.tag, 'tab' : tab , 'row' : row }
    def pane_html_header(self):
        return '    <div id="%s" class="tab-pane fade">'%self.tag
    def pane_html_footer(self):
        return '    </div>  <!-- %s -->\n'%self.tag

    def html(self):
        s = self.pane_html_header()
        s += self.pane_html()
        s += self.pane_html_footer()
        return s
    
    def cmd(self,cmd, data=None):
        fn = 'cmd_'+cmd
        if hasattr(self, fn):
            mrflog.info( "OK you can go")
            return getattr(self,fn)(data)
        else:
            mrflog.info("you're not coming in here")
