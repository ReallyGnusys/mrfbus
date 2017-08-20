import logging
import install


class MrfLog(object):
    def __init__(self, level = install.log_level):
        formatter = logging.Formatter(fmt='%(asctime)s.%(msecs)03d] %(levelname)s %(filename)s.%(lineno)d - %(message)s' , datefmt='%Y-%m-%d,%H:%M:%S')
        self.log = logging.getLogger(install.logger_name)
        hdlr = logging.FileHandler(install.logdir+install.mrflog)
        hdlr.setFormatter(formatter)    
        self.log.addHandler(hdlr) 
        self.log.setLevel(level)  
        
    
def mrf_log():
    return logging.getLogger(install.logger_name);


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
    formatter = logging.Formatter(fmt='%(asctime)s.%(msecs)03d] %(levelname)s %(filename)s.%(lineno)d - %(message)s' , datefmt='%Y-%m-%d,%H:%M:%S')
    mrflog = logging.getLogger(install.logger_name)
    hdlr = logging.FileHandler(install.logdir+install.mrflog)
    hdlr.setFormatter(formatter)    
    mrflog.addHandler(hdlr) 
    mrflog.setLevel(level)  
    return mrflog



    ch = logging.StreamHandler()
    ch.setFormatter(formatter)
    ch.setLevel(level)
    alog.addHandler(ch) 
    logging.addLevelName( logging.WARNING, "\033[1;31m%s\033[1;0m" % logging.getLevelName(logging.WARNING))
    logging.addLevelName( logging.ERROR, "\033[1;41m%s\033[1;0m" % logging.getLevelName(logging.ERROR))
    return alog
   
mrflog = mrf_log_init()

"""
def debug(*args, **kwargs):
    log = mrf_log_init()
    log.debug(*args, **kwargs)

def info(*args, **kwargs):
    log = mrf_log_init()
    log.info(*args, **kwargs)

def warn(*args, **kwargs):
    log = mrf_log_init()
    log.warn(*args, **kwargs)

def error(*args, **kwargs):
    log = mrf_log_init()
    log.error(*args, **kwargs)
    

"""
