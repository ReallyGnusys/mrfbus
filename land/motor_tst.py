from motor import motor_tornado
from tornado.ioloop import IOLoop

from tornado import gen


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

docdate = datetime.datetime.combine(datetime.date(year=2020,month=5,day=25),datetime.time())

sensor_id = 'LOUNGE_AMBIENT'
stype = 'temp'
coll = db.get_collection('sensors.%s.%s'%(stype,sensor_id))
print "coll repr is %s"%repr(coll)

doc = new_sensor_day_doc(sensor_id, docdate)
#print "doc is %s"%repr(doc)
doc =  { 'tag' : 'L1_AMB' , 'temp' : 23.43 , 'date' : '2017-11-20T00:12:42.232945' }



#IOLoop.current().run_sync(lambda: do_insert_doc(collection=coll,doc=doc))


#IOLoop.current().run_sync(lambda: do_update_doc(collection=coll,doc=doc, docdate=docdate, hour=0, minute=0))


IOLoop.current().run_sync(lambda: do_insert(sensor_id=sensor_id, stype=stype, docdate=docdate, hour=23, minute=23, value=255.55))
