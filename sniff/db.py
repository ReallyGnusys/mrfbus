from collections import OrderedDict
import psycopg2
import sys
from xbus_types import XB_MAXPKTLEN

from datetime import datetime
debug = False
def Connection():
    try:
        return psycopg2.connect(database="pbus", user="pbus", password="pbuspass", host="taz")
    except:
        print "Connection ERROR"
        return None
def TableName(dev,msg):
    return "dx%02X_mx%02X"%(dev,msg)

def DbgTableName(dev):
    return "dx%02X_debug"%(dev)          

def TableExists(cur,name):
    cur.execute("select * from information_schema.tables where table_name=%s", (name,))
    return bool(cur.rowcount)

def FilterDateFields(tablename):  # example
    query = "select * from dx10_mx02 where CAST(EXTRACT(minutes FROM time) AS INT) % 5 = 0;"
    return query


def MinTempOnDateExample():
    query = "select min(temperature) from dx10_mx02 where date =  date '2012-08-22';"

def MinTemps():
    query = "select * from dx10_mx02 where temperature = (select min(temperature) from dx10_mx02);";

def ResultsForHour():
    return None
    '''
    query = "create or replace function hourents ( date ,  time ) returns table(t1 time,temp real) as $$
begin
return query select time, temperature from dx10_mx02 where date = $1 and time >= $2 and time <= $2 + interval '59 minutes 59 seconds';
end
$$ LANGUAGE plpgsql;"
'''

class db:
    def __init__(self,debug = None):
        self.debug = debug
        self.conn = Connection()
        if self.conn == None:
            print "db connection error"
            return
        self.cur = self.conn.cursor()
    
 

    def RowDef(self,msg = 0,createtable = False):
        if ( msg == 2 ):
            if createtable or self.debug:
                return "date date, time time , temperature real , vcc real, dbgid int DEFAULT 0"
            else:
                return "date date, time time , systime time, temperature real , vcc real"
        elif ( msg == 5 ): #meteo1
            if createtable or self.debug:
                return "date date, time time , temperature real , rh real, kpa real, fcc real, dbgid int DEFAULT 0"
            else:
                return "date date, time time , systime time, temperature real , rh real, kpa real,vcc real"

        elif ( msg == 0x18 ): #meteo1 coeffs
            if createtable or self.debug:
                return "sysdate date, systime time , a0 real , b1 real,  b2 real, c12 real,a0_raw int,b1_raw int,b2_raw int,c12_raw int, dbgid int DEFAULT 0"
            else:
                return "sysdate date, systime time , a0 real , b1 real,  b2 real, c12 real,a0_raw int,b1_raw int,b2_raw int,c12_raw int"


            
        elif ( msg == 0x10 ):
            if createtable or self.debug:
                return "sysdate date, systime time , vcc real , dbgid int DEFAULT 0"
            else:
                return "sysdate date, systime time , vcc real "
  
        elif (msg == 0): #debug row
            if createtable:
                st = "rid integer PRIMARY KEY DEFAULT nextval('dbgid'),"
            else:
                st = ""
            rv = st+"sysdate date, systime time , _buff varchar(%d) , RXRSSI int , RXLQI int , vcc real"%XB_MAXPKTLEN
            #print "RowDef msg %d createtable %d debug %d returning %s"%(msg,createtable,
                         #                                               self.debug,rv)
            
            
            return rv

        else:
            return None
    def RowVars(self,msg = 0):
        rvars = []
        tlist = self.RowDef(msg).split(",")
        for item in tlist:
            tmp = item.lstrip().rstrip()
            items = tmp.split()
            vname = items[0].rstrip().lstrip()
            rvars.append(vname)
        rstr = ""
        if len(rvars) < 1:
            print "ERROR rowvars - rvars = %s"%rvars
            return "ERROR RowVars"
        elif len(rvars) == 1:
            rstr += str(rvars[0])
        else:
            for i in range(0,len(rvars) -1):
                rstr += str(rvars[i]) +","
            rstr += str(rvars[len(rvars) -1])
        return rstr
        
    def RowDict(self,msg=0):
        if self.RowDef(msg) == None:        
            #print "no rowdef for msg 0x%02X"%msg
            return None
        tlist = self.RowDef(msg).split(",")
        rdict = OrderedDict()
        for item in tlist:
            tmp = item.lstrip().rstrip()
            items = tmp.split()
            vname = items[0].rstrip().lstrip()
            vval  = items[1].rstrip().lstrip()
            rdict[vname] = vval
        return rdict



    def CreateTableIfNotExists(self,xbmsg):
        dev = xbmsg.hdr.source
        msg = xbmsg.hdr.type
        
        name = TableName(dev,msg)
        if TableExists(self.cur,name):
            if self.debug:
                print "CreateMsg2Table : %s already exists"%name
            return 0
        rowdef = self.RowDef(msg)
        if rowdef == None:
            print "CreateMsg2Table : no rowdef for message 0x%X"%(msg)
            return -1
        print "Creating table %s"%name
        query = "CREATE TABLE %s (%s);"%(name , rowdef)
        self.cur.execute(query)
        return 1
    def _row_variables_str(self,rowdic):
        l1 = len(rowdic)
        if len(rowdic) < 1:
            return ""
        elif len(rowdic) == 1:
            return "%s"
        else:
            st = ""
            for i in range(0,len(rowdic)-1):
                st = st + "%s, "
            st = st + "%s"
            return st
    def buildvalues(self,msg,rowdict):
        vals = []
        for name in rowdict.keys():            
            vals.append(getattr(msg,name))
        
        return vals
    '''
    print "looking for value for %s - type %s"%(name,rowdict[name])
            if name == sysdate:
                vals.append("sysdate")
            elif name == systime:
       '''
    
    def DebugEntry(self,xbmsg):
        dev = xbmsg.hdr.source
        debugtable = DbgTableName(dev)
        if not TableExists(self.cur,debugtable):
            rowdef = self.RowDef(createtable=True)

            query = "CREATE SEQUENCE dbgid START 1;"
            self.cur.execute(query)

            query = "CREATE TABLE %s (%s);"%(debugtable , rowdef)
            print "creating debug table with query %s"%query
            self.cur.execute(query)
            print "Debug table %s created for device %d"%(debugtable,dev)

 
        query = "INSERT INTO %s (%s) VALUES (%s) RETURNING rid"%(debugtable,self.RowVars(), self._row_variables_str(self.RowDict()))
        values = self.buildvalues(xbmsg,self.RowDict())

        print "running query %s with values %s"%(query,values)
        self.cur.execute(query, values)
        rid = self.cur.fetchone()[0]

        print "rid is %d"%rid
        return rid
    def Insert(self,xbmsg):
        if self.debug:
            xbmsg.dbgid = self.DebugEntry(xbmsg)
        else:
            xbmsg.dgbid = None

            
        dev = xbmsg.hdr.source
        msg = xbmsg.hdr.type
        table = TableName(dev,msg)
        rdict = self.RowDict(msg)

        if rdict == None:
            return
        
        self.CreateTableIfNotExists(xbmsg)
        query = "INSERT INTO %s (%s) VALUES (%s)"%(table,self.RowVars(msg), self._row_variables_str(rdict))
        values = self.buildvalues(xbmsg,rdict)
        #print "running query %s"%query
        #print "with values   %s"%values
        self.cur.execute(query, values)
        self.conn.commit()
'''
conn = Connection()

if conn == None:
    print "could not connect"
    sys.exit(-1)

dev = 16
msg = 2

cur = conn.cursor()

CreateDevMsgTable(cur,dev,msg)
'''
