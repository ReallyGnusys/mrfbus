import logging
import install

def mrf_log():
    return logging.getLogger(install.logger_name);

mrflog=None

"""
def logdeco(f):
    @wraps(f)
    def wrapped(*args, **kwargs):
        r = f(*args, **kwargs)
        return r
    return wrapped

@logdeco
def mrflog(*args, **kwargs)
"""           

def mrf_log_init(level = install.log_level):
    global mrflog
    formatter = logging.Formatter(fmt='%(asctime)s.%(msecs)03d] %(levelname)s %(filename)s.%(lineno)d - %(message)s' , datefmt='%Y-%m-%d,%H:%M:%S')
    alog = logging.getLogger(install.logger_name)
    hdlr = logging.FileHandler(install.logdir+install.mrflog)
    hdlr.setFormatter(formatter)    
    alog.addHandler(hdlr) 
    alog.setLevel(level)  
    mrflog = alog
    return alog
    ch = logging.StreamHandler()
    ch.setFormatter(formatter)
    ch.setLevel(level)
    alog.addHandler(ch) 
    logging.addLevelName( logging.WARNING, "\033[1;31m%s\033[1;0m" % logging.getLevelName(logging.WARNING))
    logging.addLevelName( logging.ERROR, "\033[1;41m%s\033[1;0m" % logging.getLevelName(logging.ERROR))
    return alog
   


