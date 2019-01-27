import logging
import install
import datetime as dt




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
class MyFormatter(logging.Formatter):
    converter=dt.datetime.fromtimestamp
    def formatTime(self, record, datefmt=None):
        ct = self.converter(record.created)
        if datefmt:
            s = ct.strftime(datefmt)
        else:
            t = ct.strftime("%Y-%m-%d %H:%M:%S")
            s = "%s.%03d" % (t, record.msecs)
        return s


def mrf_log_init(level = install.log_level):
    #logging.basicConfig(filename=install.logdir+install.mrflog,level=level)
    formatter = logging.Formatter(fmt='%(asctime)s] %(levelname)s %(filename)s.%(lineno)d - %(message)s')# , datefmt='%Y-%m-%d,%H:%M:%S')
    #formatter = MyFormatter(fmt='%(asctime)s.%(msecs)03d] %(levelname)s %(filename)s.%(lineno)d - %(message)s', datefmt='%Y-%m-%d,%H:%M:%S')
    mrflog = logging.getLogger(install.logger_name)
    hdlr = logging.FileHandler(install.logdir+install.mrflog)
    hdlr.setFormatter(formatter)

    mrflog.addHandler(hdlr)

    #ch = logging.StreamHandler()
    #ch.setFormatter(formatter)
    #mrflog.addHandler(ch)
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
