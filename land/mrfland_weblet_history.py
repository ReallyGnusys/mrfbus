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
from mrfland_weblet import MrflandWeblet, mrfctrl_butt_html
from mrflog import mrflog
from mrfland import to_json

class MrfLandWebletHistory(MrflandWeblet):
    def init(self):
        mrflog.info("%s init"%(self.__class__.__name__))

    def cmd_mrfctrl(self,data):
        mrflog.warn( "cmd_mrfctrl here, data was %s"%repr(data))

        if data['tab'] != 'date_range':
            return

        sl = self.rm.server.db_sensors

        stype = sl.keys()[0]
        sensor_id = sl[stype][0]
        
        if data['fld'] == 'day':
            
            mrflog.warn( "gen day graph ")
            self.rm.server.sensor_db_day_doc(sensor_id=sensor_id,stype=stype)
    
    def pane_html(self):

        sens = self.rm.server.db_sensors
        mrflog.warn("pane_html got sensors %s"%repr(sens))
        s =  """
        <h2>%s</h2>nobbit"""%self.label
        s += """<hr>
        """+ mrfctrl_butt_html(self.tag,'date_range','day','day',cls="")+"""
<script type="text/javascript">

"""+to_json(sens)+"""
</script>


"""
        return s


