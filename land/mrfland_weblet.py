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

def MrflandObjectTable(app,tname, odict,rows):
    """ utility to generate a default html table to display an ordered dict """
    s = """
        <table class="table app-%s tab-%s">
          <thead>
            <tr>"""%(app,tname)
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
        for fld in odict.keys():
            s += """
            <td class="app-%s tb-%s rw-%s fld-%s"></td>"""%(app,tname,str(row),fld)
        s += """
            </tr>"""
    s += """
           </tbody>
         </table>
"""
    return s


class MrflandWeblet(object):  
    def __init__(self, rm, log, data):
        if not data.has_key('tag'):
            log.error('weblet data does not contain tag')
            return
        if not data.has_key('label'):
            log.error('weblet data does not contain label')
            return
        
        log.info("creating weblet tag %s label %s"%(data['tag'],data['label']))
        self.rm = rm
        self.tag = data['tag']
        self.label = data['label']
        self.log = log
        self.data = data
        if hasattr(self, 'post_init'):
            self.post_init()
            
    def pane_html_header(self):
        return '    <div id="%s" class="tab-pane fade">'%self.tag
    def pane_html_footer(self):
        return '    </div>  <!-- %s -->\n'%self.tag

    def html(self):
        s = self.pane_html_header()
        s += self.pane_html()
        s += self.pane_html_footer()
        return s


    def pane_js_header(self):
        return """
var mrf_weblet_%s = new function() {
  this.cmd = {
"""%self.tag
    
    def pane_js_footer(self):
        return """}; //  mrf_weblet_%s \n
"""%self.tag

    def js(self):
        s = self.pane_js_header()
        s += self.pane_js_cmd()
        s += "};\n"
        s += self.pane_js()
        s += self.pane_js_footer()
        return s


    def setlog(self,log):
        self.log = log

    
