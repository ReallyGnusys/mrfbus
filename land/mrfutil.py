import datetime
import json
import mrflog

DateTimeFormat = '%Y-%m-%dT%H:%M:%S'

def mob_handler(obj):
    if isinstance(obj, datetime.datetime): #or isinstance(obj, date)
        return obj.isoformat()
    elif isinstance(obj, datetime.time): #or isinstance(obj, date)
        return str(obj)
    elif isinstance(obj, bool): #or isinstance(obj, date)
        return str(obj)
    else:
        return obj
"""
dt_handler = lambda obj: (
    obj.isoformat()
    if isinstance(obj, datetime)
    or isinstance(obj, date)
    else None)
"""


def to_json(obj):
    try:
        js = json.dumps(obj,default = mob_handler)
        return js
    except:
        mrflog.error("to_json error obj was %s"%repr(obj))
        return "{}"

def json_parse(str):
    try:
        ob = json.loads(str)
    except:
        return None
    return ob
