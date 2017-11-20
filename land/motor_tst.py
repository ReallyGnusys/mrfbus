from motor import motor_tornado
from tornado.ioloop import IOLoop

from tornado import gen


from mrflog import mrflog
uri = "mongodb://mrfbus:sanghamnamami@bolt:27017/mrfbus?authSource=admin"


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
def do_insert_doc(**kwargs):

    col = kwargs['collection']
    doc = kwargs['doc']    

    future = col.insert_one(doc)
    result = yield future
    print "result was %s"%repr(result)


@gen.coroutine
def do_update_doc(**kwargs):

    col = kwargs['collection']
    docdate = kwargs['docdate']    
    minute = kwargs['minute']
    hour = kwargs['hour']

    future = col.update({'docdate':docdate},
               { "$set" : { 'data.0.0' : 55.9 } })
    #future = col.insert_one(doc)
    result = yield future


    if result['updatedExisting']:
        mrflog.info("doc update result good %s "%repr(result))
    else:
        mrflog.info("result dodgy %s "%repr(result))
        
        
@gen.coroutine
def do_insert(**kwargs):

    sensor_id = kwargs['sensor_id']
    docdate = kwargs['docdate']    
    minute = kwargs['minute']
    hour = kwargs['hour']
    value  = kwargs['value']


    coll = db.get_collection('sensors.%s'%sensor_id)

    future = coll.update({'docdate':docdate},
               { "$set" : { 'data.0.0' : value } })
    #future = col.insert_one(doc)
    result = yield future


    if result['updatedExisting']:
        mrflog.warn("doc update result good %s "%repr(result))
    else:
        mrflog.warn("result dodgy %s "%repr(result))
        
        


docdate = datetime.datetime.combine(datetime.datetime.now().date(),datetime.time())

#docdate = datetime.datetime.combine(datetime.date(year=2020,month=1,day=1),datetime.time())

sensor_id = 'LOUNGE_AMBIENT'

coll = db.get_collection('sensors.%s'%sensor_id)
print "coll repr is %s"%repr(coll)

doc = new_sensor_day_doc(sensor_id, docdate)
#print "doc is %s"%repr(doc)
#doc =  { 'tag' : 'L1_AMB' , 'temp' : 23.43 , 'date' : '2017-11-20T00:12:42.232945' }



#IOLoop.current().run_sync(lambda: do_insert_doc(collection=coll,doc=doc))


#IOLoop.current().run_sync(lambda: do_update_doc(collection=coll,doc=doc, docdate=docdate, hour=0, minute=0))


IOLoop.current().run_sync(lambda: do_insert(sensor_id=sensor_id, docdate=docdate, hour=0, minute=0, value=45.76))
