from motor import motor_tornado
from tornado.ioloop import IOLoop

from tornado import gen

from mrfland import DateTimeFormat


from mrflog import mrflog


uri = "mongodb://mrfbus:sanghamnamami@bolt:27017/mrfbus?authSource=admin"

"""
clear sensor data in mongo with

for (n =0; n< cn.length;n++ ) { if (cn[n].search('^sensor.') == 0) { db.getCollection(cn[n]).drop();}

"""
client = motor_tornado.MotorClient(uri)
db = client.mrfbus
import datetime


def new_sensor_day_doc(sensor_id, docdate):
    doc = {
        'sensor_id' : sensor_id,
        'docdate' : docdate ,

        'data' : []
        
    }

    for hour in range(24):
        doc['data'].append([])
        for minute in range(60):
            doc['data'][hour].append(-273.16)
    
    return doc


@gen.coroutine
def db_get_day_doc(**kwargs):

    sensor_id = kwargs['sensor_id']
    stype = kwargs['stype']
    dt = kwargs['docdate']
    
    docdate = datetime.datetime.combine(dt.date(),datetime.time())

    coll = db.get_collection('sensor.%s.%s'%(stype,sensor_id))
    doc = yield coll.find_one({'docdate' : docdate})
    #mrflog.warn("got cursor %s "%(repr(cursor)))


    mrflog.warn("got doc %s"%repr(doc))



def graph_day_data(doc):
    """ unwinds 2 day hour/min array data and breaks when un-initialised value found"""
    gdata = dict()

    stype = doc['stype']
    gdata[stype] = {
        'ts'   : [],
        'value': []
    }
        
    gvals =  gdata[stype]
    gtime = doc['docdate']
    
    for hour in xrange(24):
        for minute in xrange(60):
            """
            if blank and (doc['data'][hour][minute] == blank):  # return early
                indnext = hour*60+minute+1
                indprev = hour*60+minute-1
                if (indnext/60 > 23):
                    return gdata
                if (indprev/60 < 0):
                    return gdata
                
                if (doc['data'][indnext/60][indnext%60] != blank): #pad last valid if only one missing value
                    gdata.append(doc['data'][indprev/60][indprev%60])
                
            else:
            """
            if doc['data'][hour][minute] == doc['nullval']:
                print "got nullval hour %d min %d val %s"%(hour,minute,repr(doc['data'][hour][minute]))
                return gvals
            
            gvals['ts'].append(gtime.strftime(DateTimeFormat))
            gvals['value'].append(doc['data'][hour][minute])
            gtime = gtime + datetime.timedelta(minutes=1)
    return gvals


        
@gen.coroutine
def db_day_graph(**kwargs):

    sensor_ids = kwargs['sensor_ids']
    stype = kwargs['stype']
    dt = kwargs['docdate']
    
    docdate = datetime.datetime.combine(dt.date(),datetime.time())

    gdata = {}
    
    for sensor_id in sensor_ids:
        coll = db.get_collection('sensor.%s.%s'%(stype,sensor_id))
        doc = yield coll.find_one({'docdate' : docdate})

        mrflog.warn("got doc for %s  %s"%(sensor_id,repr(doc)))
        gdata[sensor_id] = graph_day_data(doc)
        mrflog.warn("gdata %s %s"%(sensor_id,repr(gdata)))

    mrflog.warn("gdata %s"%repr(gdata))
    
    
    
    
@gen.coroutine
def db_get_sensors(**kwargs):


    cnames = yield db.collection_names()
    

    mrflog.warn("got cnames %s"%repr(cnames))
    
    sensors = dict()

    for cn in cnames:
        fld = cn.split('.')
        if (fld[0] == 'sensor') and (len(fld) == 3):
            stype = fld[1]
            sname = fld[2]
            if not sensors.has_key(stype):
                sensors[stype] = []
            sensors[stype].append(sname)

    for stype in sensors.keys():
        sensors[stype].sort()
        
    mrflog.warn("got sensors %s"%repr(sensors))

        
@gen.coroutine
def do_insert(**kwargs):

    sensor_id = kwargs['sensor_id']
    stype = kwargs['stype']
    docdate = kwargs['docdate']    
    minute = kwargs['minute']
    hour = kwargs['hour']
    value  = kwargs['value']


    coll = db.get_collection('sensor.%s.%s'%(stype,sensor_id))

    future = coll.update({'docdate':docdate},
               { "$set" : { 'data.'+str(hour)+'.'+str(minute) : value } })
    #future = col.insert_one(doc)
    result = yield future


    if result['updatedExisting']:
        mrflog.warn("doc update result good %s "%repr(result))
    else:
        mrflog.warn("result dodgy %s "%repr(result))
        doc = new_sensor_day_doc(sensor_id, docdate)
        future = coll.insert_one(doc)
        result = yield future
        mrflog.warn("insert result was %s"%repr(result))
        future = coll.update({'docdate':docdate},
                             { "$set" : { 'data.0.0' : value } })
        #future = col.insert_one(doc)
        result = yield future
        if result['updatedExisting']:
            mrflog.warn("doc update(2) result good %s "%repr(result))
        else:
            mrflog.error("result(2) bad %s "%repr(result))
        
        

docdate = datetime.datetime.combine(datetime.datetime.now().date(),datetime.time())


sensor_id = 'LOUNGE_AMBIENT'
stype = 'temp'
coll = db.get_collection('sensors.%s.%s'%(stype,sensor_id))
print "coll repr is %s"%repr(coll)






#IOLoop.current().run_sync(lambda: do_insert(sensor_id=sensor_id, stype=stype, docdate=docdate, hour=23, minute=23, value=255.55))



IOLoop.current().run_sync(lambda: db_get_day_doc(sensor_id=sensor_id, stype=stype, docdate=docdate))

IOLoop.current().run_sync(lambda: db_get_sensors())
IOLoop.current().run_sync(lambda: db_day_graph(sensor_ids=[sensor_id,'OUTSIDE_AMBIENT'], stype=stype, docdate=docdate))
