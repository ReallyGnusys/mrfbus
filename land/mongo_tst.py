import pymongo

from install import db_uri
from mrfland import sensor_null_val

client = pymongo.MongoClient(db_uri)

db = client.mrfbus

cns = db.collection_names()


def check_doc_data(doc):
    #print "check_data nv %s got data ..e.g. %s"%(repr(nullval),repr(data[0][0]))

    data = doc['data']
    nullval = doc['nullval']
    cnull = data[0][0] == nullval
    pval = data[0][0]
    mod = False
    for hour in xrange(24):
        for minute in xrange(60):
            ni = hour*60 + minute + 1
            pi = hour*60 + minute - 1

            if ni/60 < 24:
                nv = data[ni/60][ni%60]
                nn = (nv == nullval)
            else:
                nv = None
                nn = False
                
            if pi/60 < 24:
                pv = data[pi/60][pi%60]
                pn = (pv == nullval)
            else:
                pv = None
                pn = False
                
            cn = data[hour][minute] == nullval
            if cn and not nn and not pn and (pv != None ) and (nv != None):
                print "%s %s check_data anomaly hour %d minute %d data %s"%(doc['sensor_id'],doc['docdate'],hour,minute,repr(data[hour][minute]))
                data[hour][minute] = (nv + pv) / 2.0
                print "%s %s mod data %s"%(doc['sensor_id'],doc['docdate'],repr(data[hour][minute]))
                mod = True
    if mod:
        return doc
    else:
        return None
    
for cn in cns:
    if cn.find("sensor.") != 0:
        continue
    fld = cn.split(".")

    stype = fld[1]
    sensor_id = fld[2]
    print "stype %s sensor_id %s"%(stype,sensor_id)
    col = db.get_collection(cn)
    cur = col.find()

    for doc in cur:
        #print doc['_id']
        d2 = col.find_one({'_id': doc['_id'] })
        #print d2
        if not d2.has_key('stype') and not d2.has_key('null_val'):
            print "not migrated!! %s"%d2['_id']
            d2["stype"] = stype
            d2["nullval"] =  sensor_null_val(stype)
            #print "replacing with"
            #print d2
            res = col.replace_one({'_id': doc['_id'] },d2)
            #print "matched "+repr(res.matched_count)
            if res.modified_count > 0:            
                print d2
                print "modified "+repr(res.modified_count)

 
        fdoc = check_doc_data(d2)

        if fdoc:
            print "doc fixed"
            fdoc2 = check_doc_data(fdoc)
            if fdoc2:
                print "Warning not fixed"
            else:
                res = col.replace_one({'_id': doc['_id'] },fdoc)
                #print "matched "+repr(res.matched_count)
                if res.modified_count > 0:            
                    print d2
                    print "modified "+repr(res.modified_count)
