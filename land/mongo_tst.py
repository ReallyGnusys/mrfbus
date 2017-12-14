import pymongo

from install import db_uri
from mrfland import sensor_null_val, sensor_round_val

client = pymongo.MongoClient(db_uri)

db = client.mrfbus

cns = db.collection_names()


def check_doc_null_data(doc):
    #print "check_data nv %s got data ..e.g. %s"%(repr(nullval),repr(data[0][0]))

    data = doc['data']
    nullval = doc['nullval']
    cnull = data[0][0] == nullval
    pval = data[0][0]
    mod = False
    for hour in xrange(24):
        for minute in xrange(60):

            dv = data[hour][minute]
            pi = hour*60 + minute - 1

                
            if pi/60 >= 0:
                pv = data[pi/60][pi%60]
                pn = (pv == nullval)
            else:
                pv = None
                pn = False
                
            cn = (dv == nullval)

            for n in xrange(4):
                ni = hour*60 + minute + n
                if ni/60 < 24:
                    nv = data[ni/60][ni%60]
                    nn = (nv == nullval)
                else:
                    nv = None
                    nn = False

                if cn and not nn and not pn and (pv != None ) and (nv != None):
                    print "%s %s check_data anomaly hour %d minute %d data %s"%(doc['sensor_id'],doc['docdate'],hour,minute,repr(data[hour][minute]))
                    val = (nv + pv) / dv.__class__(2)
                    if doc.has_key('max_dp'):
                        val = round(val,doc['max_dp'])
                    
                    data[hour][minute] = val
                    
                    print "%s %s mod data %s"%(doc['sensor_id'],doc['docdate'],repr(data[hour][minute]))
                    mod = True
    if mod:
        return doc
    else:
        return None






def check_doc_data_type(doc):
    #print "check_data nv %s got data ..e.g. %s"%(repr(nullval),repr(data[0][0]))

    data = doc['data']    
    nullval = doc['nullval']

    dtype = data[0][0].__class__.__name__  # FIXME - how do we know first one is right?
    
    mod = False
    for hour in xrange(24):
        for minute in xrange(60):

            dv = data[hour][minute]

            if dv.__class__.__name__ != dtype:

                
                changed = data[0][0].__class__(data[hour][minute])
                    
                print "%s %s  data type anomaly hour %d min %d  val %s  changed to %s"%(doc['sensor_id'],
                                                                                        doc['docdate'],hour,minute,
                                                                                        repr(data[hour][minute]),
                                                                                        repr(changed))
                data[hour][minute] = changed                                
                mod = True
    if mod:
        return doc
    else:
        return None



    


def check_doc_data_precision(doc):
    #print "check_data nv %s got data ..e.g. %s"%(repr(nullval),repr(data[0][0]))

    data = doc['data']
    mod = False
    stype = doc['stype']

    if doc.has_key('min_res'):
        del(doc['min_res'])
        print "had min_res!"
        return doc

    maxdp = sensor_round_val(stype)

    if maxdp and not doc.has_key('max_dp'):
        doc['max_dp'] = maxdp
        print "no max_dp!"
        return doc

    if doc.has_key('max_dp'):
        maxdp = doc['max_dp']
    else:
        maxdp = None
    for hour in xrange(24):
        for minute in xrange(60):
                
            cv = data[hour][minute]

            if cv.__class__ == float:
                if maxdp and (round(cv,maxdp) != cv):
                    print "%s %s check_precision anomaly hour %d minute %d data %s should be %s"%(doc['sensor_id'],doc['docdate'],hour,minute,repr(cv),repr(round(cv,maxdp)))
                    data[hour][minute] = round(cv,maxdp)
                    #data[hour][minute] = data[hour][minute] * 
                    #print "%s %s mod data %s"%(doc['sensor_id'],doc['docdate'],repr(data[hour][minute]))
                    mod = True
            #else:
            #    print "WARN float cv for %s but no max_dp!"%sensor_id
    if mod:
        return doc
    else:
        return None


"""    
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
        if (not d2.has_key('stype')) or (not d2.has_key('nullval')) or (not d2.has_key('min_res')):
            print "not migrated!! %s"%d2['_id']
            print d2
            d2["stype"] = stype
            d2["nullval"] =  sensor_null_val(stype)
            d2["min_res"] =  sensor_min_res(stype)
            #print "replacing with"
            #print d2
            res = col.replace_one({'_id': doc['_id'] },d2)
            #print "matched "+repr(res.matched_count)
            if res.modified_count > 0:            
                print d2
                print "modified "+repr(res.modified_count)
        if d2['min_res'] != sensor_min_res(stype):
            d2["min_res"] =  sensor_min_res(stype)
            res = col.replace_one({'_id': doc['_id'] },d2)
            #print "matched "+repr(res.matched_count)
            if res.modified_count > 0:            
                print "modified min_res"+repr(res.modified_count)

        fdoc = check_doc_null_data(d2)

        if fdoc:
            print "doc (null data) altered %s"%fdoc['sensor_id']
            fdoc2 = check_doc_null_data(fdoc)
            if fdoc2:
                print "Warning not fixed in one pass %s"%fdoc['sensor_id']
            else:
                res = col.replace_one({'_id': doc['_id'] },fdoc)
                #print "matched "+repr(res.matched_count)
                if res.modified_count > 0:            
                    #print d2
                    print "modified "+repr(res.modified_count)+"  null_data  "+sensor_id
        #else:
        #    print "doc (null data)not altered %s"%doc['sensor_id']


        if fdoc:
            d2 = fdoc
        fdoc = check_doc_data_precision(d2)

        if fdoc:
            print "doc fixed"
            fdoc2 = check_doc_data_precision(fdoc)
            if fdoc2:
                print "Warning not fixed"
            res = col.replace_one({'_id': doc['_id'] },fdoc)
            #print "matched "+repr(res.matched_count)
            if res.modified_count > 0:            
                #print d2
                print "modified "+repr(res.modified_count)+" data_precision "+sensor_id

        if fdoc:
            d2 = fdoc
                    
        fdoc = check_doc_data_type(d2)

        if fdoc:
            print "doc fixed"
            fdoc2 = check_doc_data_type(fdoc)
            if fdoc2:
                print "Warning not fixed"
            res = col.replace_one({'_id': doc['_id'] },fdoc)
            #print "matched "+repr(res.matched_count)
            if res.modified_count > 0:            
                #print d2
                print "modified "+repr(res.modified_count)+" data_precision "+sensor_id


                
        """
