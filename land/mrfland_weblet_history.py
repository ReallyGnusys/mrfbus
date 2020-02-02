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
from .mrfland_weblet import MrflandWeblet, mrfctrl_butt_html, mrfctrl_select_html
from .mrflog import mrflog
from .mrfland import to_json
import datetime
from .mrfland_regmanager import MrflandRegManager  # FYI self/ __MrflandWeblet__.rm is this type

class MrfLandWebletHistory(MrflandWeblet):
    _config_ = [ ('graph_days'      ,  '1' ,
                  {
                      'sel_list' :  ['1','7','28']
                  }),
                 ('graph_type'      ,  'Ambient Temps' ,
                  {
                      'sel_list' : ['Ambient Temps' , 'Store Temps']
                  }
                 )]

    def init(self):
        mrflog.info("%s init"%(self.__class__.__name__))
        self.graphid = "graph1"
        self.days = 1
        self.graph_type = 'Ambient'


    def mrfctrl_handler(self,data,wsid):
        mrflog.warn("mrfctrl_handler here, data was %s"%repr(data))

        if data['tab'] != self.graphid:
            return




        if data['row'] == 'days':
            self.days = int(data['value'])
        elif data['row'] == 'type':
            self.graph_type = data['value']

        if self.graph_type == 'Ambient':
            sensor_ids=["LOUNGE_AMBIENT","OUTSIDE_AMBIENT"]
        elif self.graph_type == 'Store':
            sensor_ids=["ACC_100","ACC_60","ACC_30","ACC_RET"]
        else:
            sensor_ids=["ACC_100","ACC_60","ACC_30","OUTSIDE_AMBIENT"]


        docdate = datetime.datetime.combine(datetime.datetime.now().date(),datetime.time())

        self.rm.db_days_graph(sensor_ids=sensor_ids,stype='temp',docdate=docdate,wsid=wsid, wtag=self.graphtag(self.graphid),days=self.days)

    def pane_html(self):

        s = """<hr>
        <div>
        """
        s += mrfctrl_select_html(self.tag,self.graphid, 'days', ['1','7','28'],'Days')
        s += mrfctrl_select_html(self.tag,self.graphid, 'type', ['Ambient','Store','Energy'],'Graph')

        s += """
      </div>
"""

        s += '<div id="'+self.graphid+'"></div>'

        if hasattr(self.rm,'db_sensors'):
            sens = self.rm.db_sensors
            mrflog.warn("pane_html got sensors %s"%repr(sens))
        else:
            sens = None

        if False:  # sens:    FIXME - what is this , generates javascript browser error - NOT VALID!!
            s += """
<script type="text/javascript">
"""+to_json(sens)+"""
</script>

"""
        return s
